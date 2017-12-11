/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { get_serial, spectrum_to_rgb, push_and_get_index, lastize, catch_all } from "./utils";
import { CellDelegate, Editor, EditorStep } from "./editor";
import { check_version, create_workset, load_workset, list_worksets, Step, Workset, Renderer, RenderingStyles } from "./workset";

let current_workset : Workset;
let current_renderer : Renderer;
const DEFAULT_STYLE : RenderingStyles = RenderingStyles.ALT_HTML;

// Whether to include non essential steps or not
let include_non_essentials : boolean = false;

$(function() {
  check_version().then(function() : void {
    $("#work_area").html(Mustache.render(WORK_AREA_TEMPL, {}));
    reload_workset_list();
  }).catch(catch_all);
});

function reload_workset_list() : void {
  list_worksets().then(function (worksets) : void {
    $("#workset_list").html(Mustache.render(WORKSET_LIST_TEMPL, {
      worksets: worksets,
    }))
  }).catch(catch_all);
}

function workset_loaded(new_workset : Workset) {
  reload_workset_list();
  current_workset = new_workset;
  current_renderer = new Renderer(DEFAULT_STYLE, current_workset);
  $("#workset").html(Mustache.render(WORKSET_TEMPL, {}));
  update_workset_globals();
}

export function get_current_workset() : Workset {
  return current_workset;
}

export function ui_create_workset() {
  create_workset().then(workset_loaded).catch(catch_all);
}

export function ui_load_workset(id : number) {
  load_workset(id).then(workset_loaded).catch(catch_all);
}

function update_workset_globals() : void {
  $("#workset_status").text(current_workset.get_human_description());
  $("#global_style").html(current_renderer.get_global_style());
}

export function ui_load_data() {
  current_workset.load_data().then(update_workset_globals).catch(catch_all);
}

function render_proof_internal(proof_tree, depth : number, step : number) : [string, number] {
  let children : [string, number][] = proof_tree.children.filter(function(el) {
    return include_non_essentials ? true : el["essential"];
  }).map(function (el) {
    let proof : string;
    [proof, step] = render_proof_internal(el, depth+1, step);
    return [proof, step];
  });
  step += 1;
  return [Mustache.render(PROOF_STEP_TEMPL, {
    label: current_workset.labels[proof_tree.label],
    label_tok: proof_tree.label,
    number: proof_tree.number > 0 ? proof_tree.number.toString() : "",
    number_color: spectrum_to_rgb(proof_tree.number, current_workset.max_number),
    sentence: current_renderer.render_from_codes(proof_tree.sentence),
    children: children.map(function(el) { return el[0]; }),
    children_steps: lastize(children.map(function(el) { return { step: el[1] }; })),
    dists: lastize(proof_tree["dists"].map(function(el) { return { dist: current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]) }; })),
    indentation: ". ".repeat(depth) + (depth+1).toString(),
    essential: proof_tree["essential"],
    step: step,
  }), step];
}

function render_proof(proof_tree) {
  return render_proof_internal(proof_tree, 0, 0)[0];
}

export function ui_show_proof_for_label(label_tok : number) {
  current_workset.do_api_request(`get_assertion/${label_tok}`).then(function(data) : Promise<void> {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : Promise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, current_workset.do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    return Promise.all(requests).then(function(responses : Array<any>) : void {
      $("#workset_area").html(Mustache.render(PROOF_TEMPL, {
        thesis: current_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
        ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
        float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
        dists: lastize(assertion["dists"].map(function(el) { return { dist: current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]) }; })),
        proof: render_proof(responses[requests_map["proof_tree"]]["proof_tree"]),
      }));
    });
  }).catch(catch_all);
}

export function ui_show_proof() {
  if (!current_workset.loaded) {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = current_workset.labels_inv[label];
  ui_show_proof_for_label(label_tok);
}

let editor : Editor;

export function get_editor() {
  return editor;
}

class ProofStepCellDelegate implements CellDelegate {
  proof_tree;
  remote_id : number;
  workset : Workset;
  step : EditorStep;
  suggestion_ready : boolean = false;

  constructor(proof_tree, workset : Workset = null, remote_id : number = -1) {
    this.proof_tree = proof_tree;
    this.workset = workset;
    this.remote_id = remote_id;
  }

  set_step(step : EditorStep) : void {
    this.step = step;
  }

  show_suggestion() : void {
    $(`.suggestion`).fadeOut();
    $(`#${this.step.get_id()}_suggestion`).fadeIn();
  }

  get_suggestion() : void {
    let self = this;
      this.show_suggestion();
    if (this.suggestion_ready) {
      return;
    }
    let label : number = this.proof_tree["label"];
    current_workset.do_api_request(`get_assertion/${label}`).then(function(data) : Promise<void> {
      let assertion = data["assertion"];

      // Request all the interesting things for the proof
      let requests : Promise<any>[] = [];
      let requests_map = {
        thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
        ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
        float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      };

      // Fire all the requests and then feed the results to the template
      return Promise.all(requests).then(function(responses : Array<any>) : void {
        $(`#${self.step.get_id()}_suggestion`).html(Mustache.render(SUGGESTION_TEMPL, {
          thesis: current_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
          ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
          float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
          dists: assertion["dists"].map(function(el) { return current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]); }),
        }));
        self.suggestion_ready = true;
      });
    }).catch(catch_all);
  }

  cell_ready() : void {
    let self = this;
    let params = {
      cell_id: this.step.get_id(),
      sentence: current_renderer.render_from_codes(this.proof_tree.sentence),
      text_sentence: new Renderer(RenderingStyles.TEXT, current_workset).render_from_codes(this.proof_tree.sentence),
      label: current_workset.labels[this.proof_tree.label],
      number: this.proof_tree.number > 0 ? this.proof_tree.number.toString() : "",
      number_color: spectrum_to_rgb(this.proof_tree.number, current_workset.max_number),
      dists: this.proof_tree["dists"].map(function(el) { return current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]); }).join("; "),
    };
    this.step.get_data1_element().append(Mustache.render(DATA1_TEMPL, params));
    this.step.get_data2_element().append(Mustache.render(DATA2_TEMPL, params));
    $(`#${this.step.get_id()}_label`).hover(function() {
      self.get_suggestion();
    }, function() {
      $(`#${self.step.get_id()}_suggestion`).fadeOut();
    });
    $(`#${this.step.get_id()}_text_input`).on("input", function() {
      let tokens : string[] = $(`#${self.step.get_id()}_text_input`).val().split(" ");
      let sentence = current_renderer.render_from_strings(tokens);
      $(`#${self.step.get_id()}_sentence`).html(sentence);
    });
  }
}

function modifier_render_proof(proof_tree, editor : Editor, parent : string) {
  if (!proof_tree["essential"] && !include_non_essentials) {
    return;
  }
  let cell_delegate = new ProofStepCellDelegate(proof_tree, current_workset);
  let editor_step = editor.create_step(parent, -1, cell_delegate);
  for (let child of proof_tree.children) {
    modifier_render_proof(child, editor, editor_step.id);
  }
}

/*function modifier_to_workset(proof_tree, workset : Workset, parent : Step) {
  if (!proof_tree["essential"] && !include_non_essentials) {
    return;
  }
  let step = workset.create_step(parent, -1, function (success : boolean) {
    if (success) {
      let promise = $.when(true);
      for (let child of proof_tree.children) {
        promise.then(function (success : boolean) {
          if (success) {

          } else {
            return false;
          }
        });
      }
    }
  });
}*/

export function ui_show_modifier() {
  if (!current_workset.loaded) {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = current_workset.labels_inv[label];
  current_workset.do_api_request(`get_assertion/${label_tok}`).then(function(data) : Promise<void> {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : Promise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, current_workset.do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    return Promise.all(requests).then(function(responses : Array<any>) : void {
      editor = new Editor("workset_area");
      modifier_render_proof(responses[requests_map["proof_tree"]]["proof_tree"], editor, editor.root_step.id);
    });
  }).catch(catch_all);
}

/*function retrieve_sentence(label_tok : number) : number[] {
  let sentence : number[];
  jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${label_tok}`, true, true, false).done(function(data) {
    sentence = data["sentence"];
  });
  return sentence;
}*/

const PROOF_TEMPL = `
  Floating hypotheses:<ol>
  {{ #float_hyps_sent }}
  <li>{{{ . }}}</li>
  {{ /float_hyps_sent }}
  </ol>
  Essential hypotheses:<ol>
  {{ #ess_hyps_sent }}
  <li>{{{ . }}}</li>
  {{ /ess_hyps_sent }}
  </ol>
  <p>Distinct variables: {{ #dists }}{{{ dist }}}{{ ^last }}; {{ /last }}{{ /dists }}</p>
  <p>Thesis: {{{ thesis }}}</p>
  <center><table summary="Proof of theorem" cellspacing="0" border="" bgcolor="#EEFFFA">
  <caption><b>Proof of theorem <b>{{ label }}</b></caption>
  <tbody>
  <tr><th>Step</th><th>Hyp</th><th>Ref</th><th>Expression</th><th>Dists</th></tr>
  {{{ proof }}}
  </tbody>
  </table></center>
`;

const PROOF_STEP_TEMPL = `
  {{ #children }}
  {{{ . }}}
  {{ /children }}
  <tr>
  <td>{{ step }}</td>
  <td>{{ #children_steps }}{{ step }}{{ ^last }}, {{ /last }}{{ /children_steps }}</td>
  <td>{{ #label_tok }}<a href="#" onclick="mmpp.ui_show_proof_for_label({{ . }})">{{ /label_tok }}{{ label }}{{ #label_tok }}</a>{{ /label_tok }} <span class="r" style="color: {{ number_color }}">{{ number }}</span></td>
  <td><span class="i">{{{ indentation }}}</span> {{{ sentence }}}</td>
  <td>{{ #dists }}{{{ dist }}}{{ ^last }}; {{ /last }}{{ /dists }}</td>
  </tr>
`;

const DATA1_TEMPL = `
  <span style="position: relative;">
    <span id="{{ cell_id }}_label" class="label">
      {{ label }} <span class="r" style="color: {{ number_color }}">{{ number }}</span>
    </span>
    <div class="outer_above">
      <div style="display: none;" id="{{ cell_id }}_suggestion" class="inner_above suggestion">
        <span class="loading">Loading...</span>
      </div>
    </div>
  </span>
  <span id="{{ cell_id }}_sentence" class="sentence">{{{ sentence }}}</span>
`;

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

const WORK_AREA_TEMPL = `
  <div id="workset_list"></div>
  <div id="workset"></div>
`;

const WORKSET_LIST_TEMPL = `
  <button onclick="mmpp.ui_create_workset()">Create new workset</button>
  {{ #worksets }}
  <button onclick="mmpp.ui_load_workset({{ id }})">{{ name }}</button>
  {{ /worksets }}
`;

const WORKSET_TEMPL = `
  <div id="workset_status"></div>
  <div id="commands">
    <button onclick="mmpp.ui_load_data()">Load database</button>
    <input type="text" id="statement_label"></input>
    <button onclick="mmpp.ui_show_proof()">Show proof</button>
    <button onclick="mmpp.ui_show_modifier()">Show modifier</button>
  </div>
  <div id="workset_area"></div>
`;
