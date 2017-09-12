/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, invert_list } from "./utils";

export class Workset {
  id : number;
  status : string;
  symbols : [string];
  labels : [string];
  symbols_inv: { [tok : string] : number };
  labels_inv: { [tok : string] : number };
  max_number : number;
  addendum;

  constructor(id : number) {
    this.id = id;
  }

  get_context(callback : () => void) {
    let self = this;
    jsonAjax(`/api/1/workset/${this.id}/get_context`, true, false).done(function(data) {
      self.status = data.status;
      if (self.status === "loaded") {
        self.symbols = data.symbols;
        self.labels = data.labels;
        self.symbols_inv = invert_list(self.symbols);
        self.labels_inv = invert_list(self.labels);
        self.addendum = data.addendum;
        self.max_number = data.max_number;
      }
      callback();
    })
  }

  load_data(callback : () => void) {
    let self = this;
    jsonAjax(`/api/1/workset/${this.id}/load`).done(function(data) {
      self.get_context(callback);
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
    if (this.workset.status !== "loaded") {
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

export function create_workset(callback : (Workset) => void) {
  jsonAjax("/api/1/workset/create").done(function(data) {
    let workset : Workset = new Workset(data.id);
    workset.get_context(function() {
      callback(workset);
    });
  });
}

export function load_workset(id : number, callback : (Workset) => void) {
  let workset : Workset = new Workset(id);
  workset.get_context(function() {
    callback(workset);
  });
}
