/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { gen_random_id } from "./utils";

export class Step {
  id : string;
  parent : Step;
  children : Array< Step >;

  constructor(id : string, parent : Step) {
    this.id = id;
    this.parent = parent;
    this.children = new Array();
  }
}

export class Editor {
  parent_div : string;
  id : string;
  root_step : Step;
  steps_map : Map< string, Step >;

  constructor(parent_div : string) {
    this.parent_div = parent_div;
    this.id = gen_random_id();
    this.steps_map = new Map();

    // Construct a dummy root step, with some glue DOM code
    let root_id = gen_random_id();
    this.root_step = new Step(root_id, null);
    this.steps_map[root_id] = this.root_step;
    let html_code = Mustache.render(STEP_TMPL, {
      eid: this.id,
      id: root_id,
      sentence: "This is the root step",
    });
    $(`#${parent_div}`).html(html_code);
    let self = this;
    $(`#${this.id}_step_${root_id}_sent`).click(function() {
      console.log(this);
      console.log(self.id, root_id);
    });
  }

  create_step(parent : string, index : number) {
    let parent_step = this.steps_map[parent];
    let new_id = gen_random_id();
    let new_step = new Step(new_id, parent_step);
    this.steps_map[new_id] = new_step;
    parent_step.children.splice(index, 0, new_step);

    // Add code to the DOM
    let self = this;
    let html_code = Mustache.render(STEP_TMPL, {
      eid: this.id,
      id: new_id,
      sentence: self.id + ":" + new_id,
    });
    if (index === 0) {
      $(`#${this.id}_step_${parent_step.id}_children`).prepend(html_code);
    } else {
      $(`#${this.id}_step_${parent_step.children[index-1].id}`).after(html_code);
    }
    $(`#${this.id}_step_${new_id}_sent`).click(function() {
      console.log(this);
      console.log(this.id, new_id);
    });
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

const STEP_TMPL = `
<div id="{{ eid }}_step_{{ id }}" class="step">
  <div id="{{ eid }}_step_{{ id }}_sent" class="step_sent_row">
    <span class="step_sent">{{{ sentence }}}</span>
  </div>
  <div id="{{ eid }}_step_{{ id }}_children" class="step_children"></div>
</div>
`;
