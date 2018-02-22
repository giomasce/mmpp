/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, assert, get_serial, spectrum_to_rgb, catch_all, push_and_get_index, tokenize } from "./utils";
import { TreeManager, TreeNode, Tree } from "./tree";
import { Workset, RenderingStyles, WorksetEventListener } from "./workset";
import { NodePainter, EditorManager } from "./tree_editor";

const API_VERSION : number = 1;

export class StepManager {
  remote_id : number;
  sentence : number[];
  searching : boolean;
  found_proof : boolean;
  proof_data : any;
  suggestion_ready : boolean;

  constructor() {
    this.sentence = [];
    this.suggestion_ready = false;
  }

  do_api_request(workset_manager : WorksetManager, url : string, data : object = null) : Promise<object> {
    return workset_manager.do_api_request(`step/${this.remote_id}/` + url, data);
  }

  reload(workset_manager : WorksetManager) : Promise< any > {
    let self = this;
    return this.do_api_request(workset_manager, `get`).then(function (data : any) : any {
      self.sentence = data.sentence;
      self.searching = data.searching;
      if (!self.searching) {
        self.found_proof = data.found_proof;
        if (self.found_proof) {
          self.proof_data = data.proof_data;
        }
      }
      return data;
    });
  }

  get_proof(workset_manager : WorksetManager) : Promise< string > {
    return this.do_api_request(workset_manager, `prove`, {}).then(function (data : any) : string {
      if (!data.success) {
        return "Could not generate proof";
      } else {
        return data.proof;
      }
    });
  }

  dump(workset_manager : WorksetManager) : Promise< string > {
    return this.do_api_request(workset_manager, `dump`, {}).then(function (data : any) : string {
      return JSON.stringify(data);
    });
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
      return step.reload(self).then(function (data : any) : Promise<void> {
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
    step.reload(this).then(function (data : any) : void {
      self.redraw_node(node);
    });
  }

  redraw_node(node : TreeNode) : void {
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let full_id = this.editor_manager.compute_full_id(node);
    if (step.searching) {
      $(`#${full_id}_label`).html(Mustache.render(LABEL_SEARCHING_TEMPL, {}));
    } else if (!step.found_proof) {
      $(`#${full_id}_label`).html(Mustache.render(LABEL_FAILED_TEMPL, {}));
    } else {
      if (step.proof_data.type == "unification") {
        this.redraw_node_unification(node);
      } else if (step.proof_data.type == "wff") {
        this.redraw_node_wff(node);
      } else {
        throw new Error("Unknown proof_data type");
      }
    }
    $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_LOADING_TEMPL, {}));
    step.suggestion_ready = false;
  }

  redraw_node_unification(node : TreeNode) : void {
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let full_id = this.editor_manager.compute_full_id(node);
    let proof_data = step.proof_data;
    let label_params = {
      label: workset.labels[proof_data.label],
      number: proof_data.number > 0 ? proof_data.number.toString() : "",
      number_color: proof_data.number > 0 ? spectrum_to_rgb(proof_data.number, workset.max_number) : "",
    };
    $(`#${full_id}_label`).html(Mustache.render(LABEL_UNIFICATION_TEMPL, label_params));
  }

  redraw_node_wff(node : TreeNode) : void {
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let full_id = this.editor_manager.compute_full_id(node);
    let proof_data = step.proof_data;
    let label_params = {
    };
    $(`#${full_id}_label`).html(Mustache.render(LABEL_WFF_TEMPL, label_params));
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
    if (step.searching) {
      $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_SEARCHING_TEMPL, {}));
      step.suggestion_ready = true;
    } else if (!step.found_proof) {
      $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_FAILED_TEMPL, {}));
      step.suggestion_ready = true;
    } else {
      if (step.proof_data.type == "unification") {
        this.get_suggestion_unification(node);
      } else if (step.proof_data.type == "wff") {
            this.get_suggestion_wff(node);
      } else {
        throw new Error("Unknown proof_data type");
      }
    }
  }

  get_suggestion_unification(node : TreeNode) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let default_renderer = workset.get_default_renderer();
    let full_id = this.editor_manager.compute_full_id(node);
    let proof_data = step.proof_data;
    workset.do_api_request(`get_assertion/${proof_data.label}`).then(function(data) : Promise<void> {
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
        $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_UNIFICATION_TEMPL, {
          thesis: default_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
          ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return default_renderer.render_from_codes(responses[el]["sentence"]); }),
          float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return default_renderer.render_from_codes(responses[el]["sentence"]); }),
          dists: assertion["dists"].map(function(el) { return default_renderer.render_from_codes([el[0]]) + ", " + default_renderer.render_from_codes([el[1]]); }),
          subst_map: proof_data.subst_map.map(function(el) { return [default_renderer.render_from_codes([el[0]]), default_renderer.render_from_codes(el[1])]; }).sort(),
        }));
        step.suggestion_ready = true;
      });
    }).catch(catch_all);
  }

  get_suggestion_wff(node : TreeNode) : void {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    let workset = this.get_workset();
    let default_renderer = workset.get_default_renderer();
    let full_id = this.editor_manager.compute_full_id(node);
    $(`#${full_id}_suggestion`).html(Mustache.render(SUGGESTION_WFF_TEMPL, {}));
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
    $(`#${full_id}_btn_get_proof`).click(this.get_proof.bind(this, node));
    $(`#${full_id}_btn_dump`).click(this.dump.bind(this, node));
  }

  get_proof(node : TreeNode) {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    step.get_proof(this).then(function (proof : string) : void {
      $(`#workset_proof`).val(proof);
    }).catch(catch_all);
  }

  dump(node : TreeNode) {
    let self = this;
    let step : StepManager = this.get_manager_object(node);
    step.dump(this).then(function (dump_data : string) : void {
      $(`#workset_proof`).val(dump_data);
    }).catch(catch_all);
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

  creating_node(node : TreeNode, special : any) : Promise< void > {
    let self = this;
    let step = new StepManager();
    this.set_manager_object(node, step);
    if (!this.loading) {
      let req_url = `step/create`;
      let req_data = {};
      if (special) {
        req_url = `step/create_from_dump`;
        req_data = {"dump": special["dump"]};
      }
      return this.do_api_request(req_url, req_data).then(function (data : any) : Promise< void > {
        step.remote_id = data.id;
        assert(!self.remote_id_map.has(step.remote_id));
        self.remote_id_map.set(step.remote_id, node.get_id());
        return step.reload(self).then(function (data : any) : void {});
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

const DATA1_TEMPL = `
  <button id="{{ cell_id }}_btn_get_proof" class="mini_button mini_button_get_proof"></button>
  <button id="{{ cell_id }}_btn_dump" class="mini_button mini_button_dump"></button>
  <span style="position: relative;">
    <span id="{{ cell_id }}_label" class="label"></span>
    <div class="outer_above">
      <div style="display: none;" id="{{ cell_id }}_suggestion" class="inner_above suggestion"></div>
    </div>
  </span>
  <span id="{{ cell_id }}_sentence" class="sentence">{{{ sentence }}}</span>
`;

const LABEL_SEARCHING_TEMPL = `<span style="color: blue;">searching</span>`;
const LABEL_FAILED_TEMPL = `<span style="color: red;">failed</span>`;
const LABEL_UNIFICATION_TEMPL = `<span class="strategy">Unif</span> {{ label }} <span class="r" style="color: {{ number_color }}">{{ number }}</span>`;
const LABEL_WFF_TEMPL = `<span class="strategy">Wff</span>`;

const SUGGESTION_SEARCHING_TEMPL = `Searching for a proof`;
const SUGGESTION_FAILED_TEMPL = `Could not find a proof`;
const SUGGESTION_LOADING_TEMPL = `<span class="loading">Loading...</span>`;
const SUGGESTION_MISSING_TEMPL = `No suggestion available`;

const SUGGESTION_UNIFICATION_TEMPL = `
  Step was proved with theorem:<br>
  &#8658; {{{ thesis }}}<br>
  {{ #ess_hyps_sent }}
  {{{ . }}}<br>
  {{ /ess_hyps_sent }}
  Distinct variables:
  {{ #dists }}
  {{{ . }}};
  {{ /dists }}
  <br>
  Substitution map:<br>
  {{ #subst_map }}
  {{{ 0 }}}: {{{ 1 }}}<br>
  {{ /subst_map }}
`;

const SUGGESTION_WFF_TEMPL = SUGGESTION_MISSING_TEMPL;

const DATA2_TEMPL = `
  <div><input type="text" id="{{ cell_id }}_text_input" class="text_input" value="{{ text_sentence }}"></input></div>
  <div>Distinct variables: {{{ dists }}}</div>
`;
