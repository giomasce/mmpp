/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, invert_list } from "./utils";

const API_VERSION : number = 1;

export class Step {
  id : number;
  workset : Workset;
  children : Array< Step >;
  sentence : number[];

  constructor(id : number, workset : Workset) {
    this.id = id;
    this.workset = workset;
    this.children = new Array();
  }

  do_api_request(url : string, data : object = null) : Promise<object> {
    return this.workset.do_api_request(`step/${this.id}/` + url, data);
  }

  load_from_remote() : Promise<void> {
    let self : Step = this;
    return this.do_api_request(`get`).then(function (x : any) : Promise<void> {
      let promise : Promise<void> = Promise.resolve();
      for (let child_id of x.children) {
        promise.then(function () : Promise<void> {
          let child : Step = new Step(child_id, self.workset);
          self.children.push(child);
          return child.load_from_remote();
        })
      }
      return promise;
    });
  }

  set_sentence(sentence : number[]) : Promise<void> {
    this.sentence = sentence;
    return this.do_api_request(`set_sentence`, {sentence: sentence.join(" ")}).then(function (data : object) : void {
    });
  }
}

export interface WorksetEventListener {
  receive_event(event : object) : void;
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
  receiving_events : boolean = false;
  receiving_stats : boolean = false;
  event_listeners : WorksetEventListener[];
  addendum;

  constructor(id : number) {
    this.id = id;
    this.styles = new Map();
    this.event_listeners = [];
  }

  do_api_request(url : string, data : object = null) : Promise<object> {
    return jsonAjax(`/api/${API_VERSION}/workset/${this.id}/` + url, data);
  }

  set_antidists(antidists : [number, number][]) : Promise<void> {
    return this.do_api_request(`set_antidists`, {antidists: antidists.map(function (x : [number, number]) : string {
      return x.join(",");
    }).join(" ")}).then(function (x : object) : void {
    });
  }

  get_renderer(style : RenderingStyles) : Renderer {
    return this.styles.get(style);
  }

  get_default_renderer() : Renderer {
    return this.get_renderer(DEFAULT_STYLE);
  }

  add_event_listener(listener : WorksetEventListener) : void {
    this.event_listeners.push(listener);
  }

  delete_event_listener(listener : WorksetEventListener) : void {
    let idx = this.event_listeners.indexOf(listener);
    if (idx >= 0) {
      this.event_listeners.splice(idx, 1);
    }
  }

  process_event(data : object) : void {
    if (!this.receiving_events) {
      return;
    }
    if (data["event"] === "nothing") {
      return;
    }
    //console.log(data);
    for (let listener of this.event_listeners) {
      listener.receive_event(data);
    }
  }

  receive_event() : void {
    let self = this;
    this.do_api_request(`queue`, {}).then(function (data : object) : void {
      self.process_event(data);
    }).catch(function (reason) : void {
      console.log("receive_event() failed because of " + reason);
    }).then(function () : void {
      if (self.receiving_events) {
        self.receive_event();
      }
    });
  }

  start_receiving_events() : void {
    this.receiving_events = true;
    this.receive_event();
  }

  stop_receiving_events() : void {
    this.receiving_events = false;
  }

  process_stats(stats : object) : void {
    $(`#workset_stats`).html("Used RAM: " + stats["current_used_ram"] + "; peak RAM: " + stats["peak_used_ram"] +
      "; processing queue: " + stats["running_coros"] + "/" + stats["queued_coros"] + "/" + stats["queued_timed_coros"]);
  }

  receive_stats() : void {
    let self = this;
    this.do_api_request(`get_stats`, {}).then(function (stats : object) : void {
      self.process_stats(stats);
    }).catch(function (reason) : void {
      console.log("receive_stats() failed because of " + reason);
    }).then(function () : void {
      setTimeout(function () : void {
        if (self.receiving_stats) {
          self.receive_stats();
        }
      }, 1000);
    });
  }

  start_receiving_stats() : void {
    this.receiving_stats = true;
    this.receive_stats();
  }

  stop_receiving_stats() : void {
    this.receiving_stats = false;
  }

  create_step(parent : Step, idx : number) : Promise<Step> {
    let self : Workset = this;
    if (this.root_step === null) {
      throw "The workset was never loaded";
    }
    if (idx === -1) {
      idx = parent.children.length;
    }
    if (idx < 0 || parent.children.length < idx) {
      throw "Wrong index";
    }
    return this.do_api_request(`step/create`, {}).then(function (data : any) : Promise<Step> {
      let step_id : number = data.id;
      return self.do_api_request(`step/${step_id}/reparent`, {parent: parent.id, index: idx}).then(function (data : any) : Promise< Step > {
        if (!data.success) {
          throw "Failed to reparent new step";
        } else {
          let new_step = new Step(step_id, self);
          parent.children.splice(idx, 0, new_step);
          return new_step.load_from_remote().then(function () : Step {
            return new_step;
          });
        }
      });
    });
  }

  get_root_step() : Step {
    return this.root_step;
  }

  load_from_remote(load_steps : boolean) : Promise<void> {
    let self = this;
    return this.do_api_request(`get_context`).then(function(data : any) : Promise<void> {
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
      self.styles.set(RenderingStyles.HTML, new Renderer(RenderingStyles.HTML, self));
      self.styles.set(RenderingStyles.ALT_HTML, new Renderer(RenderingStyles.ALT_HTML, self));
      self.styles.set(RenderingStyles.TEXT, new Renderer(RenderingStyles.TEXT, self));
      self.styles.set(RenderingStyles.LATEX, new Renderer(RenderingStyles.LATEX, self));
      if (load_steps) {
        self.root_step = new Step(data.root_step_id, self);
        return self.root_step.load_from_remote();
      } else {
        self.root_step = null;
        return Promise.resolve();
      }
    });
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

  load_data() : Promise<void> {
    let self = this;
    return this.do_api_request(`load`).then(function(data : any) : Promise<void> {
      return self.load_from_remote(false);
    });
  }

  // FIXME Temporary workaround
  reload_with_steps() : Promise<Workset> {
    let workset : Workset = new Workset(this.id);
    return workset.load_from_remote(true).then(function() : Workset {
      return workset;
    });
  }
}

export enum RenderingStyles {
  HTML,
  ALT_HTML,
  LATEX,
  TEXT,
}

const DEFAULT_STYLE : RenderingStyles = RenderingStyles.ALT_HTML;

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

  parse_from_strings(tokens : string[]) : number[] {
    let ret : number[] = [];
    for (let tok of tokens) {
      let resolved : number = this.workset.symbols_inv[tok];
      if (resolved === undefined) {
        return null;
      } else {
        ret.push(resolved);
      }
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
          ret += ` \\textrm{${tok}} `;
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

export function check_version() : Promise<void> {
  return jsonAjax(`/api/version`).then(function(data : any) : void {
    if (!(data.application === "mmpp" && data.min_version <= API_VERSION && data.max_version >= API_VERSION)) {
      throw "Invalid application or version";
    }
  });
}

export function create_workset() : Promise<Workset> {
  return jsonAjax(`/api/${API_VERSION}/workset/create`).then(function(data : any) : Promise<Workset> {
    let workset : Workset = new Workset(data.id);
    return workset.load_from_remote(false).then(function() : Workset {
      return workset;
    });
  });
}

export interface WorksetDescr {
  id : number;
  name : string;
}

export function list_worksets() : Promise<Array<WorksetDescr>> {
  return jsonAjax(`/api/${API_VERSION}/workset/list`).then(function(data : any) : Array<WorksetDescr> {
    return data.worksets;
  });
}

export function load_workset(id : number) : Promise<Workset> {
  let workset : Workset = new Workset(id);
  return workset.load_from_remote(false).then(function() : Workset {
    return workset;
  });
}
