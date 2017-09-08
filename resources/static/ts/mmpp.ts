/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

let workset_id : number = undefined;
let workset_context : object = undefined;

// Whether to use html or althtml symbols
let use_alt : boolean = true;

// Whether to include non essential steps or not
let include_non_essentials : boolean = false;

function jsonAjax(url : string, dump : boolean = true, dump_content : boolean = true, async : boolean = true) : JQueryPromise<any> {
  return $.ajax({
    url: url,
    dataType: "json",
    async: async,
  }).then(function(data) {
    if (dump) {
      $("#console_text").append("\n\nRequested: " + url);
      if (dump_content) {
        $("#console_text").append("\nReceived: " + JSON.stringify(data));
      } else {
        $("#console_text").append("\nReceived: [hidden]");
      }
      $('#console_text').scrollTop($('#console_text')[0].scrollHeight);
    }
    return data;
  });
}

function dump_to_console(log : string) {
  $("#console_text").append("\n" + log);
}

$(function() {
  jsonAjax("/api/version").done(function(version_data) {
    $("#version").text(`Application is ${version_data.application}. Versions between ${version_data.min_version} and ${version_data.max_version} are supported.`);
    $("#version").css("display", "block");
    $("#create_workset").css("display", "block");
  })
});

function create_workset() {
  jsonAjax("/api/1/workset/create").done(function(data) {
    workset_id = data['id'];
    setTimeout(load_workset, 0);
  });
}

function load_workset() {
  jsonAjax(`/api/1/workset/${workset_id}`).done(function(data) {
    $("#workset_status").text(`Workset ${workset_id} loaded!`);
    $("#create_workset").css("display", "none");
    $("#workset").css("display", "block");
    get_context();
  });
}

function load_workset_0() {
  workset_id = 0;
  load_workset();
}

function invert_list(list : string[]) : object {
  let ret = {};
  for (let i in list) {
    ret[list[i]] = i;
  }
  return ret;
}

function get_context() {
  jsonAjax(`/api/1/workset/${workset_id}/get_context`, true, false).done(function(data) {
    workset_context = data;
    if (workset_context["status"] === "loaded") {
      workset_context["symbols_inv"] = invert_list(workset_context["symbols"]);
      workset_context["labels_inv"] = invert_list(workset_context["labels"]);
      if (use_alt) {
        $("#local_style").html(workset_context["addendum"]["htmlcss"]);
      } else {
        $("#local_style").html(".gifmath img { margin-bottom: -4px; };");
      }
    }
  })
}

function load_setmm() {
  jsonAjax(`/api/1/workset/${workset_id}/load`).done(function(data) {
    get_context();
  });
}

function build_html_statements(tokens : number[]) : [string, string, string] {
  let statement_text : string = "";
  let statement_html : string = "<span class=\"gifmath\">";
  let statement_html_alt : string = "<span " + workset_context["addendum"]["htmlfont"] + ">";
  for (let tok of tokens) {
    statement_html += workset_context["addendum"]["htmldefs"][tok];
    statement_html_alt += workset_context["addendum"]["althtmldefs"][tok];
    statement_text += workset_context["symbols"][tok] + " ";
  }
  statement_html += "</span>";
  statement_html_alt += "</span>";
  statement_text = statement_text.slice(0, -1);
  return [statement_text, statement_html, statement_html_alt];
}

function build_html_statement(tokens : number[]) : string {
  return build_html_statements(tokens)[use_alt ? 2 : 1];
}

function build_html_statements_from_input(tokens : string[]) : [string, string] {
  let statement_html : string = "<span class=\"gifmath\">";
  let statement_html_alt : string = "<span " + workset_context["addendum"]["htmlfont"] + ">";
  for (let tok of tokens) {
    let resolved = workset_context["symbols_inv"][tok];
    if (resolved === undefined) {
      statement_html += ` <span class=\"undefinedToken\">${tok}</span> `;
      statement_html_alt += ` <span class=\"undefinedToken\">${tok}</span> `;
    } else {
      statement_html += workset_context["addendum"]["htmldefs"][resolved];
      statement_html_alt += workset_context["addendum"]["althtmldefs"][resolved];
    }
  }
  statement_html += "</span>";
  statement_html_alt += "</span>";
  return [statement_html, statement_html_alt];
}

function show_statement() {
  if (workset_context["status"] !== "loaded") {
    return;
  }
  let label : string = $("#statement_label").val();
  let label_tok : number = workset_context["labels_inv"][label];
  jsonAjax(`/api/1/workset/${workset_id}/get_sentence/${label_tok}`).done(function(data) {
    let [statement_text, statement_html, statement_html_alt] = build_html_statements(data["sentence"]);
    $("#editor").val(statement_text);
    $("#current_statement").html(statement_html);
    $("#current_statement_alt").html(statement_html_alt);
  });
  $("#show_statement_div").css('display', 'block');
}

function retrieve_sentence(label_tok : number) : number[] {
  let sentence : number[];
  jsonAjax(`/api/1/workset/${workset_id}/get_sentence/${label_tok}`, true, true, false).done(function(data) {
    sentence = data["sentence"];
  });
  return sentence;
}

function push_and_get_index<T>(array : T[], object : T) : number {
  let index : number = array.length;
  array.push(object);
  return index;
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
    label: workset_context["labels"][proof_tree.label],
    label_num: proof_tree.label > 0 ? proof_tree.label.toString() : "",
    label_color: spectrum_to_rgb(proof_tree.label, workset_context["labels"].length),
    sentence: build_html_statement(proof_tree.sentence),
    children: children.map(function(el) { return el[0]; }),
    children_steps: children.map(function(el) { return el[1]; }),
    dists: proof_tree["dists"].map(function(el) { return build_html_statement([el[0]]) + ", " + build_html_statement([el[1]]); }),
    indentation: ". ".repeat(depth) + (depth+1).toString(),
    essential: proof_tree["essential"],
    step: step,
  }), step];
}

function render_proof(proof_tree) {
  return render_proof_internal(proof_tree, 0, 0)[0];
}

function show_assertion() {
  if (workset_context["status"] !== "loaded") {
    return;
  }

  // Resolve the label and request the corresponding assertion
  let label : string = $("#statement_label").val();
  let label_tok : number = workset_context["labels_inv"][label];
  jsonAjax(`/api/1/workset/${workset_id}/get_assertion/${label_tok}`).done(function(data) {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : JQueryXHR[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset_id}/get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset_id}/get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset_id}/get_sentence/${el}`)); }),
      float_hyps: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset_id}/get_sentence/${el}`)); }),
    };

    // Fire all the requests and then feed the results to the template
    $.when.apply($, requests).done(function() {
      let responses = arguments;
      $("#show_assertion_div").html(Mustache.render($('#assertion_templ').html(), {
        thesis: build_html_statement(responses[requests_map["thesis"]]["sentence"]),
        ess_hyps: requests_map["ess_hyps"].map(function(el) { return build_html_statement(responses[el]["sentence"]); }),
        float_hyps: requests_map["float_hyps"].map(function(el) { return build_html_statement(responses[el]["sentence"]); }),
        dists: assertion["dists"].map(function(el) { return build_html_statement([el[0]]) + ", " + build_html_statement([el[1]]); }),
        proof: render_proof(responses[requests_map["proof_tree"]]["proof_tree"]),
      }));
      $("#show_assertion_div").css('display', 'block');
    });
  });
}

function editor_changed() {
  if (workset_context["status"] !== "loaded") {
    return;
  }
  let tokens : string[] = $("#editor").val().split(" ");
  let [statement_html, statement_html_alt] = build_html_statements_from_input(tokens);
  $("#current_statement").html(statement_html);
  $("#current_statement_alt").html(statement_html_alt);
}

/* Implementation of the the function spectrumToRGB() in the official metamath;
   the color codes are copied from there, under the label "L53 empirical". */
let COLORS : number[][] = [
  [251, 0, 0],
  [247, 12, 0],
  [238, 44, 0],
  [222, 71, 0],
  [203, 89, 0],
  [178, 108, 0],
  [154, 122, 0],
  [127, 131, 0],
  [110, 136, 0],
  [86, 141, 0],
  [60, 144, 0],
  [30, 147, 0],
  [0, 148, 22],
  [0, 145, 61],
  [0, 145, 94],
  [0, 143, 127],
  [0, 140, 164],
  [0, 133, 218],
  [3, 127, 255],
  [71, 119, 255],
  [110, 109, 255],
  [137, 99, 255],
  [169, 78, 255],
  [186, 57, 255],
  [204, 33, 249],
  [213, 16, 235],
  [221, 0, 222],
  [233, 0, 172],
  [239, 0, 132],
];

function spectrum_to_rgb(color : number, max_color : number) : string {
  if (color === 0) {
    return "rgb(0, 0, 0)";
  }
  let fraction : number = (color - 1) / max_color;
  let partition : number = Math.floor(fraction * (COLORS.length-1));
  let frac_in_part : number = (fraction - partition) / (COLORS.length-1);
  let red : number = (1.0 - frac_in_part) * COLORS[partition][0] + frac_in_part * COLORS[partition+1][0];
  let green : number = (1.0 - frac_in_part) * COLORS[partition][1] + frac_in_part * COLORS[partition+1][1];
  let blue : number = (1.0 - frac_in_part) * COLORS[partition][2] + frac_in_part * COLORS[partition+1][2];
  //return "#" + Math.floor(red).toString(16) + Math.floor(green).toString(16) + Math.floor(blue).toString(16);
  return "rgb(" + Math.floor(red).toString() + ", " + Math.floor(green).toString() + ", " + Math.floor(blue).toString() + ")";
}
