/// <reference path="jquery.d.ts"/>

import { catch_all } from "./utils";

export class OpQueue {
  ops_promise : Promise< void >;

  constructor() {
    this.ops_promise = Promise.resolve();
  }

  // Enqueue an operation in the queue
  enqueue_operation(op : ()=>Promise< void >) : void {
    this.ops_promise = this.ops_promise.then(op).catch(catch_all);
  }

  // Return a promise the resolves when the last operation currently in the queue resolves;
  // so when the returned promise resolves, you know that all operations enqueued
  // at the moment you call after_queue() have finished
  after_queue() : Promise< void > {
    let self = this;
    return new Promise(function (resolve, reject) : void {
      self.enqueue_operation(function () : Promise< void > {
        resolve();
        return Promise.resolve();
      })
    });
  }
}
