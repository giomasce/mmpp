/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

let workset_id : number = undefined;
let workset_context : object = undefined;

interface Doneable {
  done(consumer): void;
}

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
  let statement_html : string = "<style type=\"text/css\">img { margin-bottom: -4px; }</style><span>";
  let statement_html_alt : string = workset_context["addendum"]["htmlcss"] + "<span " + workset_context["addendum"]["htmlfont"] + ">";
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

function build_html_statements_from_input(tokens : string[]) : [string, string] {
  let statement_html : string = "<style type=\"text/css\">img { margin-bottom: -4px; }</style><span>";
  let statement_html_alt : string = workset_context["addendum"]["htmlcss"] + "<span " + workset_context["addendum"]["htmlfont"] + ">";
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

function render_proof(proof_tree, depth : number = 0) : string {
  return Mustache.render($('#proof_templ').html(), {
    label: workset_context["labels"][proof_tree.label],
    sentence: build_html_statements(proof_tree.sentence)[2],
    children: proof_tree.children.map(function (el) { return render_proof(el, depth+1); }),
    dists: proof_tree["dists"].map(function(el) { return workset_context["addendum"]["althtmldefs"][el[0]] + ", " + workset_context["addendum"]["althtmldefs"][el[1]]; }),
    indentation: ".".repeat(depth) + (depth+1).toString(),
  });
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
        thesis: build_html_statements(responses[requests_map["thesis"]]["sentence"])[2],
        ess_hyps: requests_map["ess_hyps"].map(function(el) { return build_html_statements(responses[el]["sentence"])[2]; }),
        float_hyps: requests_map["float_hyps"].map(function(el) { return build_html_statements(responses[el]["sentence"])[2]; }),
        dists: assertion["dists"].map(function(el) { return workset_context["addendum"]["althtmldefs"][el[0]] + ", " + workset_context["addendum"]["althtmldefs"][el[1]]; }),
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
