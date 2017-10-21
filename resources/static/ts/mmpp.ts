/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { get_serial, spectrum_to_rgb, push_and_get_index, lastize } from "./utils";
import { CellDelegate, Editor, Step } from "./editor";
import { check_version, create_workset, load_workset, list_worksets, Workset, Renderer, RenderingStyles } from "./workset";

let current_workset : Workset;
let current_renderer : Renderer;
const DEFAULT_STYLE : RenderingStyles = RenderingStyles.ALT_HTML;

// Whether to include non essential steps or not
let include_non_essentials : boolean = false;

$(function() {
  check_version(function (res : boolean) {
    if (res) {
      $("#work_area").html(Mustache.render(WORK_AREA_TEMPL, {}));
      reload_workset_list();
    } else {
      $("#work_area").html("ERROR! Could not find a compatible API");
    }
  });
});

function reload_workset_list() : void {
  list_worksets(function (worksets : object) : void {
    $("#workset_list").html(Mustache.render(WORKSET_LIST_TEMPL, {
      worksets: worksets,
    }))
  })
}

export function ui_create_workset() {
  create_workset(workset_loaded);
}

export function ui_load_workset(id : number) {
  load_workset(id, workset_loaded);
}

function workset_loaded(new_workset : Workset) {
  reload_workset_list();
  current_workset = new_workset;
  current_renderer = new Renderer(DEFAULT_STYLE, current_workset);
  $("#workset").html(Mustache.render(WORKSET_TEMPL, {}));
  update_workset_status();
  $("#global_style").html(current_renderer.get_global_style());
}

function update_workset_status() : void {
  $("#workset_status").text(current_workset.get_description());
}

export function ui_load_data() {
  current_workset.load_data(function() {
    update_workset_status();
    $("#local_style").html(current_renderer.get_global_style());
  });
}

export function show_statement() {
  if (!current_workset.loaded) {
    return;
  }
  let label : string = $("#statement_label").val();
  let label_tok : number = current_workset.labels_inv[label];
  current_workset.do_api_request(`get_sentence/${label_tok}`).done(function(data) {;
    $("#editor").val(new Renderer(RenderingStyles.TEXT, current_workset).render_from_codes(data["sentence"]));
    $("#current_statement").html(new Renderer(RenderingStyles.HTML, current_workset).render_from_codes(data["sentence"]));
    $("#current_statement_alt").html(new Renderer(RenderingStyles.ALT_HTML, current_workset).render_from_codes(data["sentence"]));
  });
  $("#show_statement_div").css('display', 'block');
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
  current_workset.do_api_request(`get_assertion/${label_tok}`).done(function(data) {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : JQueryPromise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, current_workset.do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    $.when.apply($, requests).done(function() {
      let responses = arguments;
      $("#workset_area").html(Mustache.render(PROOF_TEMPL, {
        thesis: current_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
        ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
        float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
        dists: lastize(assertion["dists"].map(function(el) { return { dist: current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]) }; })),
        proof: render_proof(responses[requests_map["proof_tree"]]["proof_tree"]),
      }));

    });
  });
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
  step : Step;
  suggestion_ready : boolean = false;

  constructor(proof_tree) {
    this.proof_tree = proof_tree;
  }

  set_step(step : Step) : void {
    this.step = step;
  }

  show_suggestion() : void {
    $(`#${this.step.get_id()}_suggestion`).fadeIn();
  }

  get_suggestion() : void {
    let self = this;
    if (this.suggestion_ready) {
      this.show_suggestion();
      return;
    }
    let label : number = this.proof_tree["label"];
    current_workset.do_api_request(`get_assertion/${label}`).done(function(data) {
      let assertion = data["assertion"];

      // Request all the interesting things for the proof
      let requests : JQueryPromise<any>[] = [];
      let requests_map = {
        thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
        ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
        float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      };

      // Fire all the requests and then feed the results to the template
      $.when.apply($, requests).done(function() {
        let responses = arguments;
        $(`#${self.step.get_id()}_suggestion`).html(Mustache.render(SUGGESTION_TEMPL, {
          thesis: current_renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
          ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
          float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return current_renderer.render_from_codes(responses[el]["sentence"]); }),
          dists: assertion["dists"].map(function(el) { return current_renderer.render_from_codes([el[0]]) + ", " + current_renderer.render_from_codes([el[1]]); }),
        }));
        self.suggestion_ready = true;
        self.show_suggestion();
      });
    });
  }

  cell_ready() : void {
    let self = this;
    let params = {
      cell_id: this.step.get_id(),
      sentence: current_renderer.render_from_codes(this.proof_tree.sentence),
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
  }
}

function modifier_render_proof(proof_tree, editor : Editor, parent : string) {
  if (!proof_tree["essential"] && !include_non_essentials) {
    return;
  }
  let cell_delegate = new ProofStepCellDelegate(proof_tree);
  let step = editor.create_step(parent, -1, cell_delegate);
  for (let child of proof_tree.children) {
    modifier_render_proof(child, editor, step.id);
  }
}

export function ui_show_modifier() {
  if (!current_workset.loaded) {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = current_workset.labels_inv[label];
  current_workset.do_api_request(`get_assertion/${label_tok}`).done(function(data) {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : JQueryPromise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, current_workset.do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, current_workset.do_api_request(`get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    $.when.apply($, requests).done(function() {
      let responses = arguments;
      editor = new Editor("workset_area");
      modifier_render_proof(responses[requests_map["proof_tree"]]["proof_tree"], editor, editor.root_step.id);
    });
  });
}

export function editor_changed() {
  if (!current_workset.loaded) {
    return;
  }
  let tokens : string[] = $("#editor").val().split(" ");
  $("#current_statement").html(new Renderer(RenderingStyles.HTML, current_workset).render_from_strings(tokens));
  $("#current_statement_alt").html(new Renderer(RenderingStyles.ALT_HTML, current_workset).render_from_strings(tokens));
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
  <td>{{ label }} <span class="r" style="color: {{ number_color }}">{{ number }}</span></td>
  <td><span class="i">{{{ indentation }}}</span> {{{ sentence }}}</td>
  <td>{{ #dists }}{{{ dist }}}{{ ^last }}; {{ /last }}{{ /dists }}</td>
  </tr>
`;

const DATA1_TEMPL = `
  <span id="{{ cell_id }}_label" class="label" style="position: relative;">
    {{ label }} <span class="r" style="color: {{ number_color }}">{{ number }}</span>
    <div class="outer_above">
      <div style="display: none;" id="{{ cell_id }}_suggestion" class="inner_above suggestion"></div>
    </div>
  </span>
  <span class="sentence">{{{ sentence }}}</span>
`;

const DATA2_TEMPL = `
  <span>Distinct variables: {{{ dists }}}</span>
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
    <button onclick="mmpp.ui_show_proof()">Show assertion</button>
    <button onclick="mmpp.ui_show_modifier()">Show modifier</button>
  </div>
  <div id="workset_area"></div>
`;
