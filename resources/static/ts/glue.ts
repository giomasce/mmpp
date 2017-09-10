/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>
/// <reference path="systemjs.d.ts"/>

let mmpp : object;

// FIXME System.js is probably not being used in the right way, but so far it works.
System.config({
  packages: {
    'js': {
      main: 'mmpp.js',
      defaultExtension: 'js',
      meta: {
        "*": {
          authorization: 'yes',
          defaultExtenions: 'js',
        }
      }
    }
  }
});
System.import("./js/mmpp.js").then(function (module) {
  mmpp = module;
})
