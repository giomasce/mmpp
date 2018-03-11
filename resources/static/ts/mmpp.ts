/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { get_serial, spectrum_to_rgb, push_and_get_index, lastize, catch_all } from "./utils";
import { check_version, create_workset, load_workset, list_worksets, Step, Workset, Renderer, RenderingStyles } from "./workset";
import { Tree, TreeNode } from "./tree";
import { EditorManager } from "./tree_editor";
import { WorksetManager } from "./tree_workset";
import { show_proof_for_label } from "./render_proof";

let current_workset : Workset = null;
let current_renderer : Renderer = null;
let current_tree : Tree = null;

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
  if (current_workset !== null) {
    current_workset.stop_receiving_events();
  }
  current_workset = new_workset;
  current_renderer = current_workset.get_default_renderer();
  $("#workset").html(Mustache.render(WORKSET_TEMPL, {}));
  update_workset_globals();
  current_workset.start_receiving_events();
  load_proof_editor();
}

export function get_current_workset() : Workset {
  return current_workset;
}

export function get_current_renderer() : Renderer {
  return current_renderer;
}

export function get_current_tree() : Tree {
  return current_tree;
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

export function ui_show_proof() {
  if (!current_workset.loaded) {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = current_workset.labels_inv[label];
  show_proof_for_label(label_tok, "proof_navigator_area", include_non_essentials);
}

// The Promise<[Promise<void>]> is just a workaround for the fact that it is not possible to construct a Promise<Promise<void>> object
function render_proof_in_workset(proof_tree, workset : Workset, parent : Step) : Promise<[Promise<void>]> {
  if (!proof_tree["essential"] && !include_non_essentials) {
    let x : [Promise<void>] = [Promise.resolve()];
    return Promise.resolve(x);
  }
  return workset.create_step(parent, -1).then(function (new_step : Step) : Promise<Step> {
    return new_step.set_sentence(proof_tree["sentence"]).then(function () : Step {
      return new_step;
    });
  }).then(function (new_step : Step) : Promise<[Promise<void>]> {
    let x : [Promise<void>] = [Promise.resolve()];
    let promise : Promise<[Promise<void>]> = Promise.resolve(x);
    for (let child of proof_tree.children) {
      promise = promise.then(function(prev_completion_ : [Promise<void>]) : Promise<[Promise<void>]> {
        let prev_completion : Promise<void> = prev_completion_[0];
        return render_proof_in_workset(child, workset, new_step).then(function (this_completion_ : [Promise<void>]) : Promise<[Promise<void>]> {
          let this_completion : Promise<void> = this_completion_[0];
          // The then clause is only to convert the type
          let next_completion : Promise<void> = Promise.all([prev_completion, this_completion]).then(function() : void {});
          let next_completion_ : [Promise<void>] = [next_completion];
          return Promise.resolve(next_completion_);
        });
      });
    }
    return promise;
  });
}

export function ui_build_tree() {
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
      proof_tree: push_and_get_index(requests, current_workset.do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      fresh_workset: push_and_get_index(requests, current_workset.reload_with_steps()),
    };

    // Fire all the requests and then feed the results to the template
    return Promise.all(requests).then(function(responses : Array<any>) : Promise<[Promise<void>]> {
      let fresh_workset = responses[requests_map["fresh_workset"]];
      let x = render_proof_in_workset(responses[requests_map["proof_tree"]].proof_tree, fresh_workset, fresh_workset.get_root_step());
      return x;
    }).then(function (completion_ : [Promise<void>]) : Promise<void> {
      let completion : Promise<void> = completion_[0];
      return completion;
    });
  }).then(function () {
  }).catch(catch_all);
}

function load_proof_editor() : void {
  let workset_manager = new WorksetManager(get_current_workset());
  let editor_manager = new EditorManager("proof_editor_area", workset_manager);
  current_tree = new Tree(get_serial(), [editor_manager, workset_manager]);
  workset_manager.load_data(current_tree);
}

export function ui_create_node() : void {
  current_tree.create_node(get_serial(), false).then(function (node : TreeNode) : void {
    node.reparent(current_tree.get_root_node(), -1);
  }).catch(catch_all);
}

export function ui_create_node_from_dump() : void {
  let special = {"dump": $(`#workset_proof`).val()};
  current_tree.create_node(get_serial(), false, special).then(function (node : TreeNode) : void {
    node.reparent(current_tree.get_root_node(), -1);
  }).catch(catch_all);
}

export function ui_proof_navigator() : void {
  $(`.workset_page`).css('display', 'none');
  $(`#proof_navigator`).css('display', 'block');
}

export function ui_proof_editor() : void {
  $(`.workset_page`).css('display', 'none');
  $(`#proof_editor`).css('display', 'block');
}

const WORK_AREA_TEMPL = `
  <div id="workset_list"></div>
  <div id="workset"></div>
  <textarea id="workset_proof"></textarea>
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
    <button onclick="alert('Does not work for the moment...');">Destroy workset</button>
    <button onclick="mmpp.ui_proof_navigator()">Proof navigator</button>
    <button onclick="mmpp.ui_proof_editor()">Proof editor</button>
  </div>

  <div id="proof_navigator" class="workset_page" style="display: none;">
    <div>
      <input type="text" id="statement_label"></input>
      <button onclick="mmpp.ui_show_proof()">Show proof</button>
    </div>
    <div id="proof_navigator_area"></div>
  </div>

  <div id="proof_editor"" class="workset_page" style="display: none;">
    <div>
      <button onclick="mmpp.ui_create_node()">Create node</button>
      <button onclick="mmpp.ui_create_node_from_dump()">Create node from dump</button>
    </div>
    <div id="proof_editor_area"></div>
  </div>
`;
