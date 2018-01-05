/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { jsonAjax, assert, get_serial } from "./utils";
import { TreeManager, TreeNode, Tree } from "./tree";

const API_VERSION : number = 1;

export class StepManager {
  remote_id : number;

  do_api_request(workset_manager : WorksetManager, url : string, data : object = null) : Promise<object> {
    return workset_manager.do_api_request(`step/${this.remote_id}/` + url, data);
  }
}

export class WorksetManager extends TreeManager {
  loading : boolean;
  remote_id : number;

  constructor(remote_id : number) {
    super();
    this.remote_id = remote_id;
    this.loading = true;
  }

  load_data(tree : Tree) : Promise< void > {
    if (!this.loading) {
      return Promise.resolve();
    }

    let self = this;
    return this.do_api_request(`get_context`).then(function(data : any) : Promise< void > {
      return self._load_step(tree, null, data.root_step_id, 0);
    }).then(function () : void {
      self.loading = false;
    });
  }

  _load_step(tree : Tree, parent : TreeNode, remote_id : number, idx : number) : Promise< void > {
    let self = this;
    let node = tree.create_node(get_serial());
    let step = this.get_manager_object(node);
    step.remote_id = remote_id;
    if (parent === null) {
      tree.set_root_node(node);
    } else {
      node.reparent(parent, idx);
    }
    return step.do_api_request(this, `get`).then(function (data : any) : Promise<void> {
      let promise : Promise<void> = Promise.resolve();
      for (let [child_idx, child_id] of data.children.entries()) {
        promise.then(function () : Promise<void> {
          return self._load_step(tree, node, child_id, child_idx);
        })
      }
      return promise;
    });
  }

  creating_node(node : TreeNode) : void {
    let obj = new StepManager();
    this.set_manager_object(node, obj);
    if (!this.loading) {
      // TODO create the node on remote end
    }
  }

  destroying_node(node : TreeNode) : void {
    let obj : StepManager = this.get_manager_object(node);
    if (!this.loading) {
      // TODO destroy node on remote end
    }
  }

  do_api_request(url : string, data : object = null) : Promise<object> {
    return jsonAjax(`/api/${API_VERSION}/workset/${this.remote_id}/` + url, data);
  }
}
