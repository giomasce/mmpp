/// <reference path="jquery.d.ts"/>

let workset_id : number = undefined;

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
  });
}

function load_setmm() {
  jsonAjax(`/api/1/workset/${workset_id}/load`).done(function(data) {

  });
}
