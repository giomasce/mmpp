/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { assert } from "./utils";
import { TreeManager, TreeNode } from "./tree";

export class EditorManagerObject {
  visible : boolean = false;
}

export class EditorManager extends TreeManager {
  parent_div : string;

  constructor(parent_div : string) {
    super();
    this.parent_div = parent_div;
  }

  creating_node(node : TreeNode) : void {
    let obj = new EditorManagerObject();
    this.set_manager_object(node, obj);
    if (node.is_root()) {
      obj.visible = true;
      let html_code = Mustache.render(STEP_ROOT_TMPL, {
        eid: node.get_tree().get_id(),
        id: node.get_id(),
      });
      $(`#${this.parent_div}`).html(html_code);
    }
  }

  destroying_node(node : TreeNode) : void {
    let obj : EditorManagerObject = this.get_manager_object(node);
    if (node.is_root()) {
      obj.visible = false;
      $(`#${this.parent_div}`).html("");
    }
  }

  after_reparenting(parent : TreeNode, child : TreeNode, idx : number) : void {
    let parent_obj : EditorManagerObject = this.get_manager_object(parent);
    let child_obj : EditorManagerObject = this.get_manager_object(child);
    assert(!child_obj.visible);
    if (parent_obj.visible) {
      this.make_subtree_visible(parent, child, idx);
    }
  }

  make_subtree_visible(parent : TreeNode, child : TreeNode, idx : number) : void {
    let parent_obj : EditorManagerObject = this.get_manager_object(parent);
    let child_obj : EditorManagerObject = this.get_manager_object(child);
    assert(!child_obj.visible);
    assert(parent_obj.visible);

    child_obj.visible = true;

    let tree_id = child.get_tree().get_id();
    let child_id = child.get_id();
    let parent_id = parent.get_id();

    // Add new code to DOM
    let html_code = Mustache.render(STEP_TMPL, {
      eid: tree_id,
      id: child.get_id(),
    });
    if (idx === 0) {
      $(`#${tree_id}_step_${parent_id}_children`).prepend(html_code);
    } else {
      let prev_child_id = parent.get_child(idx-1).get_id();
      $(`#${tree_id}_step_${prev_child_id}`).after(html_code);
    }

    // Recur on children
    for (let [idx2, child2] of child.get_children().entries()) {
      this.make_subtree_visible(child, child2, idx2);
    }
  }
}

const STEP_ROOT_TMPL = `
<div id="{{ eid }}_step_{{ id }}" class="step">
  <div id="{{ eid }}_step_{{ id }}_children"></div>
</div>
`;

const STEP_TMPL = `
<div id="{{ eid }}_step_{{ id }}" class="step">
  <div id="{{ eid }}_step_{{ id }}_row" class="step_row">
    <div id="{{ eid }}_step_{{ id }}_handle" class="step_handle">
      <button id="{{ eid }}_step_{{ id }}_btn_toggle_children" class="mini_button"></button>
      <button id="{{ eid }}_step_{{ id }}_btn_toggle_data2" class="mini_button"></button>
      <button id="{{ eid }}_step_{{ id }}_btn_close_all_children" class="mini_button"></button>
    </div>
    <div id="{{ eid }}_step_{{ id }}_data" class="step_data">
      <div id="{{ eid }}_step_{{ id }}_data1" class="step_data1"></div>
      <div id="{{ eid }}_step_{{ id }}_data2" class="step_data2" style="display: none;"></div>
    </div>
  </div>
  <div id="{{ eid }}_step_{{ id }}_children" class="step_children"></div>
</div>
`;
