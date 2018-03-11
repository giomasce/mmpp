/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { assert } from "./utils";

export class TreeManager {
  manager_id : number;

  set_manager_id(manager_id : number) : void {
    this.manager_id = manager_id;
  }

  get_manager_id() : number {
    return this.manager_id;
  }

  get_manager_object(node : TreeNode) : any {
    return node.get_manager_object(this.get_manager_id());
  }

  set_manager_object(node : TreeNode, obj : any) : void {
    node.set_manager_object(this.get_manager_id(), obj);
  }

  creating_node(node : TreeNode, special : any) : Promise< void > {
    return Promise.resolve();
  }

  destroying_node(node : TreeNode) : Promise< void > {
    return Promise.resolve();
  }

  before_reparenting(parent : TreeNode, child : TreeNode, idx : number) {
  }

  before_orphaning(parent : TreeNode, child : TreeNode, idx : number) {
  }

  after_reparenting(parent : TreeNode, child : TreeNode, idx : number) {
  }

  after_orphaning(parent : TreeNode, child : TreeNode, idx : number) {
  }
}

export class TreeNode {
  id : number;
  tree : Tree;
  parent : TreeNode;
  children : TreeNode[];
  manager_objects : Map< number, any >;

  constructor(id : number, tree : Tree) {
    this.id = id;
    this.tree = tree;
    this.parent = null;
    this.children = [];
    this.manager_objects = new Map();
  }

  get_id() : number {
    return this.id;
  }

  get_tree() : Tree {
    return this.tree;
  }

  get_child(idx : number) : TreeNode {
    return this.children[idx];
  }

  get_children() : TreeNode[] {
    return this.children;
  }

  get_parent() : TreeNode {
    return this.parent;
  }

  find_child_idx(node : TreeNode) : number {
    return this.children.findIndex(function (value : TreeNode) : boolean {
      return node === value;
    });
  }

  get_manager_object(id : number) : any {
    return this.manager_objects.get(id);
  }

  set_manager_object(id : number, obj : any) : void {
    this.manager_objects.set(id, obj);
  }

  is_root() : boolean {
    return this === this.tree.get_root_node();
  }

  null_tree() : void {
    this.tree = null;
    this.id = null;
  }

  orphan() : void {
    // Check assertions
    let parent = this.parent;
    assert(parent !== null);
    let idx = parent.find_child_idx(this);
    assert(0 <= idx);
    assert(idx < parent.children.length);

    // Call before listeners
    for (let manager of this.tree.managers) {
      manager.before_orphaning(parent, this, idx);
    }

    // Do orphaning
    parent.children.splice(idx, 1);
    this.parent = null;

    // Call after listeners
    for (let manager of this.tree.managers) {
      manager.after_orphaning(parent, this, idx);
    }
  }

  reparent(parent : TreeNode, idx : number) : void {
    if (idx === -1) {
      idx = parent.children.length;
    }
    // Check assertions
    assert(this.parent === null);
    assert(idx >= 0);
    assert(idx <= parent.children.length);
    assert(!this.is_root());

    // Call before listeners
    for (let manager of this.tree.managers) {
      manager.before_reparenting(parent, this, idx);
    }

    // Do reparenting
    parent.children.splice(idx, 0, this);
    this.parent = parent;

    // Call after listeners
    for (let manager of this.tree.managers) {
      manager.after_reparenting(parent, this, idx);
    }
  }
}

export class Tree {
  id : number;
  node_map : Map< number, TreeNode >;
  root_node : TreeNode;
  managers : TreeManager[];

  constructor(id : number, managers : TreeManager[]) {
    this.id = id;
    this.node_map = new Map();
    this.root_node = null;
    this.managers = managers;
    for (let [i, manager] of this.managers.entries()) {
      manager.set_manager_id(i);
    }
  }

  get_id() : number {
    return this.id;
  }

  create_node(id : number, is_root : boolean, special : any = null) : Promise< TreeNode > {
    let node = new TreeNode(id, this);
    assert(!this.node_map.has(id));
    this.node_map.set(id, node);
    if (is_root) {
      this.root_node = node;
    }
    let ret = Promise.resolve();
    for (let manager of this.managers) {
      ret = ret.then(function () : Promise< void > {
        return manager.creating_node(node, special);
      });
    }
    return ret.then(function () : TreeNode {
      return node;
    });
  }

  destroy_node(node : TreeNode) : Promise< void > {
    let self = this;
    assert(this.node_map.has(node.id));
    assert(node.parent === null);
    assert(node.children.length === 0);
    let ret = Promise.resolve();
    for (let manager of this.managers) {
      ret = ret.then(function () : void {
        manager.destroying_node(node);
      });
    }
    return ret.then(function () : void {
      self.node_map.delete(node.id);
      node.null_tree();
    });
  }

  get_root_node() : TreeNode {
    return this.root_node;
  }

  get_node(id : number) : TreeNode {
    return this.node_map.get(id);
  }
}
