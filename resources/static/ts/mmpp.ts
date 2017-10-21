/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, get_serial, spectrum_to_rgb, push_and_get_index } from "./utils";
import { CellDelegate, Editor, Step } from "./editor";
import { create_workset, load_workset, Workset, Renderer, RenderingStyles } from "./workset";

let workset : Workset;
let renderer : Renderer;
const DEFAULT_STYLE : RenderingStyles = RenderingStyles.ALT_HTML;

// Whether to include non essential steps or not
let include_non_essentials : boolean = false;

$(function() {
  jsonAjax("/api/version").done(function(version_data) {
    $("#version").text(`Application is ${version_data.application}. Versions between ${version_data.min_version} and ${version_data.max_version} are supported.`);
    $("#version").css("display", "block");
    $("#create_workset").css("display", "block");
  })
});

export function ui_create_workset() {
  create_workset(workset_loaded);
}

export function ui_load_workset_0() {
  load_workset(0, workset_loaded);
}

function workset_loaded(new_workset : Workset) {
  workset = new_workset;
  renderer = new Renderer(DEFAULT_STYLE, workset);
  $("#workset_status").text(`Workset ${workset.id} loaded!`);
  $("#create_workset").css("display", "none");
  $("#workset").css("display", "block");
  $("#local_style").html(renderer.get_global_style());
}

export function ui_load_data() {
  workset.load_data(function() {
    $("#workset_status").text("Library data loaded!");
    $("#local_style").html(renderer.get_global_style());
  });
}

export function show_statement() {
  if (workset.status !== "loaded") {
    return;
  }
  let label : string = $("#statement_label").val();
  let label_tok : number = workset.labels_inv[label];
  jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${label_tok}`).done(function(data) {;
    $("#editor").val(new Renderer(RenderingStyles.TEXT, workset).render_from_codes(data["sentence"]));
    $("#current_statement").html(new Renderer(RenderingStyles.HTML, workset).render_from_codes(data["sentence"]));
    $("#current_statement_alt").html(new Renderer(RenderingStyles.ALT_HTML, workset).render_from_codes(data["sentence"]));
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
  return [Mustache.render($('#proof_templ').html(), {
    label: workset.labels[proof_tree.label],
    number: proof_tree.number > 0 ? proof_tree.number.toString() : "",
    number_color: spectrum_to_rgb(proof_tree.number, workset.max_number),
    sentence: renderer.render_from_codes(proof_tree.sentence),
    children: children.map(function(el) { return el[0]; }),
    children_steps: children.map(function(el) { return el[1]; }),
    dists: proof_tree["dists"].map(function(el) { return renderer.render_from_codes([el[0]]) + ", " + renderer.render_from_codes([el[1]]); }),
    indentation: ". ".repeat(depth) + (depth+1).toString(),
    essential: proof_tree["essential"],
    step: step,
  }), step];
}

function render_proof(proof_tree) {
  return render_proof_internal(proof_tree, 0, 0)[0];
}

export function show_assertion() {
  if (workset.status !== "loaded") {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = workset.labels_inv[label];
  jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${label_tok}`).done(function(data) {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : JQueryPromise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    $.when.apply($, requests).done(function() {
      let responses = arguments;
      $("#show_assertion_div").html(Mustache.render($('#assertion_templ').html(), {
        thesis: renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
        ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return renderer.render_from_codes(responses[el]["sentence"]); }),
        float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return renderer.render_from_codes(responses[el]["sentence"]); }),
        dists: assertion["dists"].map(function(el) { return renderer.render_from_codes([el[0]]) + ", " + renderer.render_from_codes([el[1]]); }),
        proof: render_proof(responses[requests_map["proof_tree"]]["proof_tree"]),
      }));
      $("#show_assertion_div").css('display', 'block');
    });
  });
}

let editor : Editor;
export function create_editor() {
  editor = new Editor("show_assertion_div");
  $("#show_assertion_div").css('display', 'block');
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.id, 0, null);
  editor.create_step(editor.root_step.children[2].id, 0, null);
  editor.create_step(editor.root_step.children[2].id, 0, null);
  editor.create_step(editor.root_step.children[2].id, 0, null);
  editor.create_step(editor.root_step.children[4].id, 0, null);
  editor.create_step(editor.root_step.children[4].id, 0, null);
  editor.create_step(editor.root_step.children[4].id, 0, null);
}

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
    jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${label}`).done(function(data) {
      let assertion = data["assertion"];

      // Request all the interesting things for the proof
      let requests : JQueryPromise<any>[] = [];
      let requests_map = {
        thesis: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${assertion["thesis"]}`)),
        ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
        float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
      };

      // Fire all the requests and then feed the results to the template
      $.when.apply($, requests).done(function() {
        let responses = arguments;
        $(`#${self.step.get_id()}_suggestion`).html(Mustache.render(SUGGESTION_TEMPL, {
          thesis: renderer.render_from_codes(responses[requests_map["thesis"]]["sentence"]),
          ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return renderer.render_from_codes(responses[el]["sentence"]); }),
          float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return renderer.render_from_codes(responses[el]["sentence"]); }),
          dists: assertion["dists"].map(function(el) { return renderer.render_from_codes([el[0]]) + ", " + renderer.render_from_codes([el[1]]); }),
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
      sentence: renderer.render_from_codes(this.proof_tree.sentence),
      label: workset.labels[this.proof_tree.label],
      number: this.proof_tree.number > 0 ? this.proof_tree.number.toString() : "",
      number_color: spectrum_to_rgb(this.proof_tree.number, workset.max_number),
      dists: this.proof_tree["dists"].map(function(el) { return renderer.render_from_codes([el[0]]) + ", " + renderer.render_from_codes([el[1]]); }).join("; "),
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

export function show_modifier() {
  if (workset.status !== "loaded") {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = workset.labels_inv[label];
  jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${label_tok}`).done(function(data) {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : JQueryPromise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    $.when.apply($, requests).done(function() {
      let responses = arguments;
      editor = new Editor("show_assertion_div");
      modifier_render_proof(responses[requests_map["proof_tree"]]["proof_tree"], editor, editor.root_step.id);
      $("#show_assertion_div").css('display', 'block');
    });
  });
}

export function editor_changed() {
  if (workset.status !== "loaded") {
    return;
  }
  let tokens : string[] = $("#editor").val().split(" ");
  $("#current_statement").html(new Renderer(RenderingStyles.HTML, workset).render_from_strings(tokens));
  $("#current_statement_alt").html(new Renderer(RenderingStyles.ALT_HTML, workset).render_from_strings(tokens));
}

/*function retrieve_sentence(label_tok : number) : number[] {
  let sentence : number[];
  jsonAjax(`/api/1/workset/${workset.id}/get_sentence/${label_tok}`, true, true, false).done(function(data) {
    sentence = data["sentence"];
  });
  return sentence;
}*/

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
