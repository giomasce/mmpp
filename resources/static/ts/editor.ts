/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { get_serial } from "./utils";

export interface CellDelegate {
  set_step(step : EditorStep) : void;
  cell_ready() : void;
}

class TrivialCellDelegate implements CellDelegate {
  set_step(step : EditorStep) : void {
  }
  cell_ready() : void {
  }
}

export class EditorStep {
  id : string;
  editor : Editor;
  parent : EditorStep;
  cell_delegate : CellDelegate;
  children : Array< EditorStep >;
  children_open : boolean = true;
  data2_open : boolean = false;

  constructor(id : string, editor : Editor, parent : EditorStep, cell_delegate : CellDelegate) {
    this.id = id;
    this.editor = editor;
    this.parent = parent;
    if (cell_delegate === null) {
      cell_delegate = new TrivialCellDelegate();
    }
    this.cell_delegate = cell_delegate;
    this.children = new Array();
    this.cell_delegate.set_step(this);
  }

  get_id() : string {
    return `${this.editor.id}_step_${this.id}`;
  }

  get_data1_element() : JQuery {
    return $(`#${this.editor.id}_step_${this.id}_data1`);
  }

  get_data2_element() : JQuery {
    return $(`#${this.editor.id}_step_${this.id}_data2`);
  }

  just_created() : void {
    let self = this;
    $(`#${this.editor.id}_step_${this.id}_btn_toggle_children`).css("background-color", "green").click(this.toggle_children.bind(this));
    $(`#${this.editor.id}_step_${this.id}_btn_toggle_data2`).css("background-color", "red").click(this.toggle_data2.bind(this));
    $(`#${this.editor.id}_step_${this.id}_btn_close_all_children`).click(function() {
      self.open_children();
      for (let child of self.children) {
        child.close_children();
      }
    });
    this.cell_delegate.cell_ready();
  }

  toggle_children() : void {
    if (this.children_open) {
      $(`#${this.editor.id}_step_${this.id}_btn_toggle_children`).css("background-color", "red");
      $(`#${this.editor.id}_step_${this.id}_children`).slideUp();
      this.children_open = false;
    } else {
      $(`#${this.editor.id}_step_${this.id}_btn_toggle_children`).css("background-color", "green");
      $(`#${this.editor.id}_step_${this.id}_children`).slideDown();
      this.children_open = true;
    }
  }

  open_children() : void {
    if (!this.children_open) {
      this.toggle_children();
    }
  }

  close_children() : void {
    if (this.children_open) {
      this.toggle_children();
    }
  }

  toggle_data2() : void {
    if (this.data2_open) {
      $(`#${this.editor.id}_step_${this.id}_btn_toggle_data2`).css("background-color", "red");
      $(`#${this.editor.id}_step_${this.id}_data2`).slideUp();
      this.data2_open = false;
    } else {
      $(`#${this.editor.id}_step_${this.id}_btn_toggle_data2`).css("background-color", "green");
      $(`#${this.editor.id}_step_${this.id}_data2`).slideDown();
      this.data2_open = true;
    }
  }

  open_data2() : void {
    if (!this.data2_open) {
      this.toggle_data2();
    }
  }

  close_data2() : void {
    if (this.data2_open) {
      this.toggle_data2();
    }
  }
}

export class Editor {
  parent_div : string;
  id : string;
  root_step : EditorStep;
  steps_map : Map< string, EditorStep >;

  constructor(parent_div : string) {
    this.parent_div = parent_div;
    this.id = get_serial();
    this.steps_map = new Map();

    // Construct a dummy root step, with some glue DOM code
    let root_id = get_serial();
    this.root_step = new EditorStep(root_id, this, null, null);
    this.steps_map[root_id] = this.root_step;
    let html_code = Mustache.render(STEP_ROOT_TMPL, {
      eid: this.id,
      id: root_id,
    });
    $(`#${parent_div}`).html(html_code);
  }

  create_step(parent : string, index : number, cell_delegate : CellDelegate) : EditorStep {
    let parent_step = this.steps_map[parent];
    if (index === -1) {
      index = parent_step.children.length;
    }
    let new_id = get_serial();
    let new_step = new EditorStep(new_id, this, parent_step, cell_delegate);
    this.steps_map[new_id] = new_step;
    parent_step.children.splice(index, 0, new_step);

    // Add code to the DOM
    let self = this;
    let html_code = Mustache.render(STEP_TMPL, {
      eid: this.id,
      id: new_id,
    });
    if (index === 0) {
      $(`#${this.id}_step_${parent_step.id}_children`).prepend(html_code);
    } else {
      $(`#${this.id}_step_${parent_step.children[index-1].id}`).after(html_code);
    }
    new_step.just_created();
    return new_step;
  }

  delete_step(id : string) {
    let step = this.steps_map[id];
    let parent_step = step.parent;
    if (parent_step === null) {
      throw "Cannot delete root step";
    }
    while (step.children.length !==0 ) {
      this.delete_step(step.children[step.children.length-1].id);
    }
    let index = parent_step.children.findIndex(function(el) { return el.id === id; });
    if (index < 0) {
      throw "Should not arrive here";
    }
    parent_step.children.splice(index, 1);
    this.steps_map.delete(id);

    // Remove code from the DOM
    $(`#${this.id}_step_${id}`).remove();
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
