/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, assert, get_serial, spectrum_to_rgb, catch_all, push_and_get_index, tokenize } from "./utils";
import { TreeManager, TreeNode, Tree } from "./tree";
import { Workset, RenderingStyles, WorksetEventListener } from "./workset";
import { NodePainter, EditorManager } from "./tree_editor";

const API_VERSION : number = 1;

export class StepManager {
  remote_id : number;
  success : boolean;
  sentence : number[];
  label : number;
  number : number;
  suggestion_ready : boolean = false;

  do_api_request(workset_manager : WorksetManager, url : string, data : object = null) : Promise<object> {
    return workset_manager.do_api_request(`step/${this.remote_id}/` + url, data);
  }
}

export class WorksetManager extends TreeManager implements NodePainter, WorksetEventListener {
  loading : boolean;
  workset : Workset;
  ops_promise : Promise< void >;
  remote_id_map : Map< number, number >;
  tree : Tree;
  editor_manager : EditorManager;

  constructor(workset : Workset) {
    super();
    this.workset = workset;
    this.loading = true;
    this.ops_promise = Promise.resolve();
    this.remote_id_map = new Map();
    workset.add_event_listener(this);
  }

  enqueue_operation(op : ()=>Promise< void >) : void {
    this.ops_promise = this.ops_promise.then(op).catch(catch_all);
  }

  after_queue() : Promise< void > {
    let self = this;
    return new Promise(function (resolve, reject) : void {
      self.enqueue_operation(function () : Promise< void > {
        resolve();
        return Promise.resolve();
      })
    });
  }

  set_editor_manager(editor_manager : EditorManager) : void {
    this.editor_manager = editor_manager;
  }

  get_workset() : Workset {
    return this.workset;
  }

  load_data(tree : Tree) : Promise< void > {
    if (!this.loading) {
      return Promise.resolve();
    }

    this.tree = tree;

    let self = this;
    return this.do_api_request(`get_context`).then(function(data : any) : Promise< void > {
      return self._load_step(tree, null, data.root_step_id, 0);
    }).then(function () : void {
      self.loading = false;
    });
  }

  _load_step(tree : Tree, parent : TreeNode, remote_id : number, idx : number) : Promise< void > {
    let self = this;
    return tree.create_node(get_serial(), parent === null).then(function (node : TreeNode) : Promise< void > {
      let step : StepManager = self.get_manager_object(node);
      step.remote_id = remote_id;
      assert(!self.remote_id_map.has(remote_id));
      self.remote_id_map.set(remote_id, node.get_id());
      step.sentence = [];
      return step.do_api_request(self, `get`).then(function (data : any) : Promise<void> {
        step.sentence = data.sentence;
        step.success = data.success;
        if (step.success) {
          step.label = data.label;
          step.number = data.number;
        }
        let promise : Promise<void> = Promise.resolve();
        for (let [child_idx, child_id] of data.children.entries()) {
          promise = promise.then(function () : Promise<void> {
            return self._load_step(tree, node, child_id, child_idx);
          })
        }
        return promise;
      }).then(function () : void {
        if (parent !== null) {
          node.reparent(parent, idx);
        }
      });
    });
  }

  receive_event(event : object) : void {
    if (event["event"] === "step_updated") {
      let remote_id : number = event["step_id"];
      if (this.remote_id_map.has(remote_id)) {
        let node_id = this.remote_id_map.get(remote_id);
        let node = this.tree.get_node(node_id);
        this.update_node(node);
      }
    }
  }

  update_node(node : TreeNode) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    step.do_api_request(this, `get`).then(function (data : any) : void {
      step.success = data.success;
      step.suggestion_ready = false;
      if (step.success) {
        step.label = data.label;
        step.number = data.number;
      }
      self.redraw_node(node);
    });
  }

  redraw_node(node : TreeNode) : void {
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let full_id = this.editor_manager.compute_full_id(node);
    let label_params = {
      label: step.success ? workset.labels[step.label] : "",
      number: step.success && step.number > 0 ? step.number.toString() : "",
      number_color: step.success ? spectrum_to_rgb(step.number, workset.max_number) : "",
    };
    let loading_sugg_params = {
    };
    $(`#${full_id}_label`).html(Mustache.render(LABEL_TEMPL, label_params));
    $(`#${full_id}_suggestion`).html(Mustache.render(LOADING_SUGG_TEMPL, loading_sugg_params));
  }

  show_suggestion(node : TreeNode) : void {
    let full_id = this.editor_manager.compute_full_id(node);
    $(`.suggestion`).fadeOut();
    $(`#${full_id}_suggestion`).fadeIn();
  }

  get_suggestion(node : TreeNode) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    this.show_suggestion(node);
    if (step.suggestion_ready) {
      return;
    }

    let workset = this.get_workset();
    let default_renderer = workset.get_default_renderer();
    let full_id = this.editor_manager.compute_full_id(node);
    workset.do_api_request(`get_assertion/${step.label}`).then(function(data) : Promise<void> {
      let assertion = data["assertion"];

      // Request all the interesting things for the proof
      let requests : Promise<any>[] = [];
      let requests_map = {
        thesis: push_and_get_index(requests, workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
        ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, workset.do_api_request(`get_sentence/${el}`)); }),
        float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, workset.do_api_request(`get_sentence/${el}`)); }),
      };

      // Fire all the requests and then feed the results to the template
      return Promise.all(requests).then(function(responses : Array<any>) : void {
        $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_TEMPL, {
          thesis: default_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
          ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return default_renderer.render_from_codes(responses[el]["sentence"]); }),
          float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return default_renderer.render_from_codes(responses[el]["sentence"]); }),
          dists: assertion["dists"].map(function(el) { return default_renderer.render_from_codes([el[0]]) + ", " + default_renderer.render_from_codes([el[1]]); }),
        }));
        step.suggestion_ready = true;
      });
    }).catch(catch_all);
  }

  paint_node(node : TreeNode) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let default_renderer = workset.get_default_renderer();
    let text_renderer = workset.get_renderer(RenderingStyles.TEXT);
    let full_id = this.editor_manager.compute_full_id(node);
    let params = {
      cell_id: full_id,
      sentence: default_renderer.render_from_codes(step.sentence),
      text_sentence: text_renderer.render_from_codes(step.sentence),
      dists: "", //this.proof_tree["dists"].map(function(el) { return default_renderer.render_from_codes([el[0]]) + ", " + default_renderer.render_from_codes([el[1]]); }).join("; "),
    };
    this.editor_manager.compute_data1_element(node).append(Mustache.render(DATA1_TEMPL, params));
    this.editor_manager.compute_data2_element(node).append(Mustache.render(DATA2_TEMPL, params));
    this.redraw_node(node);
    $(`#${full_id}_label`).hover(function() {
      self.get_suggestion(node);
    }, function() {
      $(`#${full_id}_suggestion`).fadeOut();
    });
    $(`#${full_id}_text_input`).on("input", function() {
      let tokens : string[] = tokenize($(`#${full_id}_text_input`).val());
      let parsed_tokens : number[] = default_renderer.parse_from_strings(tokens);
      let sentence : string;
      if (parsed_tokens === null) {
        sentence = default_renderer.render_from_strings(tokens);
      } else {
        self.set_sentence(node, parsed_tokens);
        sentence = default_renderer.render_from_codes(parsed_tokens);
      }
      $(`#${full_id}_sentence`).html(sentence);
    });
  }

  set_sentence(node : TreeNode, sentence : number[]) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    step.sentence = sentence;
    this.enqueue_operation(function () : Promise< void > {
      return step.do_api_request(self, `set_sentence`, {sentence: sentence.join(" ")}).then(function (data : object) : void {
      });
    });
  }

  creating_node(node : TreeNode) : Promise< void > {
    let self = this;
    let step = new StepManager();
    this.set_manager_object(node, step);
    if (!this.loading) {
      return this.do_api_request(`step/create`, {}).then(function (data : any) : Promise< void > {
        step.remote_id = data.id;
        assert(!self.remote_id_map.has(step.remote_id));
        self.remote_id_map.set(step.remote_id, node.get_id());
        return step.do_api_request(self, `get`).then(function (data : any) : void {
          step.sentence = data.sentence;
          step.success = data.success;
          if (step.success) {
            step.label = data.label;
            step.number = data.number;
          }
        });
      });
    } else {
      return Promise.resolve();
    }
  }

  destroying_node(node : TreeNode) : Promise< void > {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    let remote_id = step.remote_id;
    assert(this.remote_id_map.has(step.remote_id));
    this.remote_id_map.delete(step.remote_id);
    assert(!this.loading);
    return this.after_queue().then(function () : Promise< void > {
      return self.do_api_request(`step/${remote_id}/destroy`, {}).then(function (data : any) : void {
        if (!data.success) {
          throw "Failed to destroy step";
        }
      });
    });
  }

  after_reparenting(parent : TreeNode, child : TreeNode, idx : number) : void {
    if (!this.loading) {
      let self = this;
      let child_step : StepManager = this.get_manager_object(child);
      let parent_step : StepManager = this.get_manager_object(parent);
      let child_remote_id = child_step.remote_id;
      let parent_remote_id = parent_step.remote_id;
      this.enqueue_operation(function () : Promise< void > {
        return self.do_api_request(`step/${child_remote_id}/reparent`, {parent: parent_remote_id, index: idx}).then(function (data : any) : void {
          if (!data.success) {
            throw "Failed to reparent step";
          }
        });
      });
    }
  }

  before_orphaning(parent : TreeNode, child : TreeNode, idx : number) : void {
    assert(!this.loading);
    let self = this;
    let child_step : StepManager = this.get_manager_object(child);
    let child_remote_id = child_step.remote_id;
    this.enqueue_operation(function () : Promise< void > {
      return self.do_api_request(`step/${child_remote_id}/orphan`, {}).then(function (data : any) : void {
        if (!data.success) {
          throw "Failed to orphan step";
        }
      });
    });
  }

  do_api_request(url : string, data : object = null) : Promise<object> {
    return this.get_workset().do_api_request(url, data);
  }
}

/*export class WorksetPainter extends TreeManager {
  workset_manager : WorksetManager;

  constructor(workset_manager : WorksetManager) {
    super();
    this.workset_manager = workset_manager;
  }
}*/

const DATA1_TEMPL = `
  <span style="position: relative;">
    <span id="{{ cell_id }}_label" class="label"></span>
    <div class="outer_above">
      <div style="display: none;" id="{{ cell_id }}_suggestion" class="inner_above suggestion"></div>
    </div>
  </span>
  <span id="{{ cell_id }}_sentence" class="sentence">{{{ sentence }}}</span>
`;

const LABEL_TEMPL = `{{ label }} <span class="r" style="color: {{ number_color }}">{{ number }}</span>`;

const LOADING_SUGG_TEMPL = `<span class="loading">Loading...</span>`;

const DATA2_TEMPL = `
  <div><input type="text" id="{{ cell_id }}_text_input" class="text_input" value="{{ text_sentence }}"></input></div>
  <div>Distinct variables: {{{ dists }}}</div>
`;

const SUGGESTION_TEMPL = `
  &#8658; {{{ thesis }}}<br>
  {{ #ess_hyps_sent }}
  {{{ . }}}<br>
  {{ /ess_hyps_sent }}
  Distinct variables:
  {{ #dists }}
  {{{ . }}};
  {{ /dists }}
`;
