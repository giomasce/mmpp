/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { gen_random_id } from "./utils";

export class Step {
  id : string;
  eid : string;
  parent : Step;
  children : Array< Step >;
  open : boolean = true;

  constructor(id : string, eid : string, parent : Step) {
    this.id = id;
    this.eid = eid;
    this.parent = parent;
    this.children = new Array();
  }

  get_data_element() : JQuery {
    return $(`#${this.eid}_step_${this.id}_data`);
  }

  just_created() : void {
    $(`#${this.eid}_step_${this.id}_handle`).html("O").click(this.toggle.bind(this));
  }

  toggle() : void {
    if (this.open) {
      $(`#${this.eid}_step_${this.id}_handle`).html("C");
      $(`#${this.eid}_step_${this.id}_children`).slideUp();
      this.open = false;
    } else {
      $(`#${this.eid}_step_${this.id}_handle`).html("O");
      $(`#${this.eid}_step_${this.id}_children`).slideDown();
      this.open = true;
    }
  }

  render() : void {
    let content = this.get_content();
    $(`#${this.eid}_step_${this.id}_data`).html(content);
    let self = this;
    $(`#${this.eid}_step_${this.id}_row`).click(function() {
      console.log(this);
      console.log(self.get_content());
    });
  }

  get_content() : string {
    return `${this.eid}<br>${this.id}`;
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
    this.root_step = new Step(root_id, this.id, null);
    this.steps_map[root_id] = this.root_step;
    let html_code = Mustache.render(STEP_TMPL, {
      eid: this.id,
      id: root_id,
    });
    $(`#${parent_div}`).html(html_code);
  }

  create_step(parent : string, index : number) : Step {
    let parent_step = this.steps_map[parent];
    if (index === -1) {
      index = parent_step.children.length;
    }
    let new_id = gen_random_id();
    let new_step = new Step(new_id, this.id, parent_step);
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

const STEP_TMPL = `
<div id="{{ eid }}_step_{{ id }}" class="step">
  <div id="{{ eid }}_step_{{ id }}_row" class="step_row">
    <div id="{{ eid }}_step_{{ id }}_handle" class="step_handle"></div>
    <div id="{{ eid }}_step_{{ id }}_data" class="step_data"></div>
  </div>
  <div id="{{ eid }}_step_{{ id }}_children" class="step_children"></div>
</div>
`;
