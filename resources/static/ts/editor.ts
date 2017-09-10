/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { gen_random_id } from "./utils";

export class Editor {
  parent_div : string;
  id : string;
  steps_map;

  constructor(parent_div : string) {
    this.parent_div = parent_div;
    this.id = gen_random_id();
    this.steps_map = {};
  }
}
