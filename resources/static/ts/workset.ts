/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, invert_list } from "./utils";

const API_VERSION : number = 1;

export class Step {
  id : number;
  workset : Workset;
  children : Array< Step >;

  constructor(id : number, workset : Workset) {
    this.id = id;
    this.workset = workset;
    this.children = new Array();
  }

  do_api_request(url : string, dump : boolean = true, dump_content : boolean = true, async : boolean = true) : JQueryPromise<any> {
    return this.workset.do_api_request(`step/${this.id}/` + url, dump, dump_content, async);
  }

  load_from_remote(callback : () => void) : void {
    let self = this;
    this.do_api_request(`get`).done(function (data) {
      callback();
    });
  }
}

export class Workset {
  id : number;
  name : string;
  loaded : boolean;
  symbols : [string];
  labels : [string];
  symbols_inv: { [tok : string] : number };
  labels_inv: { [tok : string] : number };
  max_number : number;
  styles : Map< RenderingStyles, Renderer >;
  root_step : Step;
  addendum;

  constructor(id : number) {
    this.id = id;
    this.styles = new Map();
  }

  do_api_request(url : string, dump : boolean = true, dump_content : boolean = true, async : boolean = true) : JQueryPromise<any> {
    return jsonAjax(`/api/${API_VERSION}/workset/${this.id}/` + url, dump, dump_content, async);
  }

  load_from_remote(callback : () => void) : void {
    let self = this;
    this.do_api_request(`get_context`, true, false).done(function(data) {
      self.name = data.name;
      if (data.status === "loaded") {
        self.loaded = true;
        self.symbols = data.symbols;
        self.labels = data.labels;
        self.symbols_inv = invert_list(self.symbols);
        self.labels_inv = invert_list(self.labels);
        self.addendum = data.addendum;
        self.max_number = data.max_number;
      } else {
        self.loaded = false;
      }
      // Stupid TypeScript has no sane way to iterate over an enum
      self.styles[RenderingStyles.HTML] = new Renderer(RenderingStyles.HTML, self);
      self.styles[RenderingStyles.ALT_HTML] = new Renderer(RenderingStyles.ALT_HTML, self);
      self.styles[RenderingStyles.TEXT] = new Renderer(RenderingStyles.TEXT, self);
      self.styles[RenderingStyles.LATEX] = new Renderer(RenderingStyles.LATEX, self);
      self.root_step = new Step(data.root_step_id, self);
      self.root_step.load_from_remote(callback);
    })
  }

  get_human_description() : string {
    let ret : string = this.name + ": ";
    if (this.loaded) {
      ret += `database contains ${this.labels.length} labels and ${this.symbols.length} symbols`;
    } else {
      ret += "not loaded";
    }
    return ret;
  }

  load_data(callback : () => void) {
    let self = this;
    this.do_api_request(`load`).done(function(data) {
      self.load_from_remote(callback);
    });
  }
}

export enum RenderingStyles {
  HTML,
  ALT_HTML,
  LATEX,
  TEXT,
}

export class Renderer {
  style : RenderingStyles;
  workset : Workset;

  constructor(style : RenderingStyles, workset : Workset) {
    this.style = style;
    this.workset = workset;
  }

  get_global_style() : string {
    if (!this.workset.loaded) {
      return "";
    }
    if (this.style === RenderingStyles.ALT_HTML) {
      return this.workset.addendum.htmlcss;
    } else if (this.style == RenderingStyles.HTML) {
      return ".gifmath img { margin-bottom: -4px; };";
    } else {
      return "";
    }
  }

  render_from_codes(tokens : number[]) : string {
    let ret : string;
    if (this.style === RenderingStyles.ALT_HTML) {
      ret = `<span ${this.workset.addendum.htmlfont}>`;
      for (let tok of tokens) {
        ret += this.workset.addendum.althtmldefs[tok];
      }
      ret += "</span>";
    } else if (this.style === RenderingStyles.HTML) {
      ret = "<span class=\"gifmath\">";
      for (let tok of tokens) {
        ret += this.workset.addendum.htmldefs[tok];
      }
      ret += "</span>";
    } else if (this.style === RenderingStyles.LATEX) {
      ret = "";
      for (let tok of tokens) {
        ret += this.workset.addendum.latexdefs[tok];
      }
    } else if (this.style === RenderingStyles.TEXT) {
      ret = "";
      for (let tok of tokens) {
        ret += this.workset.symbols[tok] + " ";
      }
      ret = ret.slice(0, -1);
    }
    return ret;
  }

  render_from_strings(tokens : string[]) : string {
    let ret : string;
    if (this.style === RenderingStyles.ALT_HTML) {
      ret = `<span ${this.workset.addendum.htmlfont}>`;
      for (let tok of tokens) {
        let resolved : number = this.workset.symbols_inv[tok];
        if (resolved === undefined) {
          ret += ` <span class=\"undefinedToken\">${tok}</span> `;
        } else {
          ret += this.workset.addendum.althtmldefs[resolved];
        }
      }
      ret += "</span>";
    } else if (this.style === RenderingStyles.HTML) {
      ret = "<span class=\"gifmath\">";
      for (let tok of tokens) {
        let resolved : number = this.workset.symbols_inv[tok];
        if (resolved === undefined) {
          ret += ` <span class=\"undefinedToken\">${tok}</span> `;
        } else {
          ret += this.workset.addendum.htmldefs[resolved];
        }
      }
      ret += "</span>";
    } else if (this.style === RenderingStyles.LATEX) {
      ret = "";
      for (let tok of tokens) {
        let resolved : number = this.workset.symbols_inv[tok];
        if (resolved === undefined) {
          ret += ` \textrm{${tok}} `;
        } else {
          ret += this.workset.addendum.latexdefs[tok];
        }
      }
    } else if (this.style === RenderingStyles.TEXT) {
      ret = "";
      for (let tok of tokens) {
        ret += tok + " ";
      }
      ret = ret.slice(0, -1);
    }
    return ret;
  }
}

export function check_version(callback : (boolean)=>void) : void {
  jsonAjax(`/api/version`).done(function(data) {
    if (data.application === "mmpp" && data.min_version <= API_VERSION && data.max_version >= API_VERSION) {
      callback(true);
    } else {
      callback(false);
    }
  });
}

export function create_workset(callback : (Workset) => void) {
  jsonAjax(`/api/${API_VERSION}/workset/create`).done(function(data) : void {
    let workset : Workset = new Workset(data.id);
    workset.load_from_remote(function() {
      callback(workset);
    });
  });
}

export function list_worksets(callback : (object) => void) : void {
  jsonAjax(`/api/${API_VERSION}/workset/list`).done(function(data) : void {
    callback(data.worksets);
  });
}

export function load_workset(id : number, callback : (Workset) => void) : void {
  let workset : Workset = new Workset(id);
  workset.load_from_remote(function() {
    callback(workset);
  });
}
