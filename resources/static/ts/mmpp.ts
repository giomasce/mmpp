/// <reference path="jquery.d.ts"/>

$(function() {
  $.ajax("/api/version", {
    "dataType": "json"
  }).done(function(version_data) {
    $("#version").text(`Application is ${version_data.application}. Versions between ${version_data.min_version} and ${version_data.max_version} are supported.`);
  })
});
