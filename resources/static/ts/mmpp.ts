/// <reference path="jquery.d.ts"/>

let workset_id : number = undefined;
let workset_context : object = undefined;

function jsonAjax(url : string) {
  return $.ajax(url, {
    "dataType": "json"
  });
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
  jsonAjax(`/api/1/workset/${workset_id}/get_context`).done(function(data) {
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
  let statement_html : string = workset_context["htmlcss"] + "<span class=\"math\">";
  let statement_html_alt : string = workset_context["htmlcss"] + "<span class=\"math\">";
  for (let tok of tokens) {
    statement_html += workset_context["htmldefs"][tok];
    statement_html_alt += workset_context["althtmldefs"][tok];
    statement_text += workset_context["symbols"][tok] + " ";
  }
  statement_html += "</span>";
  statement_html_alt += "</span>";
  statement_text = statement_text.slice(0, -1);
  return [statement_text, statement_html, statement_html_alt];
}

function build_html_statements_from_input(tokens : string[]) : [string, string] {
  let statement_html : string = workset_context["htmlcss"] + "<span class=\"math\">";
  let statement_html_alt : string = workset_context["htmlcss"] + "<span class=\"math\">";
  for (let tok of tokens) {
    let resolved = workset_context["symbols_inv"][tok];
    if (resolved === undefined) {
      statement_html += ` <span class=\"undefinedToken\">${tok}</span> `;
      statement_html_alt += ` <span class=\"undefinedToken\">${tok}</span> `;
    } else {
      statement_html += workset_context["htmldefs"][resolved];
      statement_html_alt += workset_context["althtmldefs"][resolved];
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
  })
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
