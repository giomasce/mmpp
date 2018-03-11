/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

import { spectrum_to_rgb, lastize, push_and_get_index, catch_all, get_udid } from "./utils";
import { get_current_renderer, get_current_workset } from "./mmpp";

function render_proof_internal(proof_tree, depth : number, step : number, div_id : string, include_non_essentials : boolean) : [string, number, [()=>void]] {
  let children : [string, number, [()=>void]][] = proof_tree.children.filter(function(el) {
    return include_non_essentials ? true : el["essential"];
  }).map(function (el) : [string, number, [()=>void]] {
    let proof : string;
    let callbacks : [()=>void];
    [proof, step, callbacks] = render_proof_internal(el, depth+1, step, div_id, include_non_essentials);
    return [proof, step, callbacks];
  });
  step += 1;
  let udid = get_udid();
  let callback_list : [()=>void] = [function() : void {
    $(`#${udid}`).click(function() {
      show_proof_for_label(proof_tree.label, div_id, include_non_essentials);
    });
  }];
  for (let el of children) {
    $.merge(callback_list, el[2]);
  }
  return [Mustache.render(PROOF_STEP_TEMPL, {
    label: get_current_workset().labels[proof_tree.label],
    label_tok: proof_tree.label,
    number: proof_tree.number > 0 ? proof_tree.number.toString() : "",
    number_color: spectrum_to_rgb(proof_tree.number, get_current_workset().max_number),
    sentence: get_current_renderer().render_from_codes(proof_tree.sentence),
    children: children.map(function(el) { return el[0]; }),
    children_steps: lastize(children.map(function(el) { return { step: el[1] }; })),
    dists: lastize(proof_tree["dists"].map(function(el) { return { dist: get_current_renderer().render_from_codes([el[0]]) + ", " + get_current_renderer().render_from_codes([el[1]]) }; })),
    indentation: ". ".repeat(depth) + (depth+1).toString(),
    essential: proof_tree["essential"],
    step: step,
    udid: udid,
  }), step, callback_list];
}

function render_proof(proof_tree, div_id : string, include_non_essentials : boolean) : [string, [()=>void]] {
  let internal = render_proof_internal(proof_tree, 0, 0, div_id, include_non_essentials);
  return [internal[0], internal[2]];
}

export function show_proof_for_label(label_tok : number, div_id : string, include_non_essentials : boolean) {
  get_current_workset().do_api_request(`get_assertion/${label_tok}`).then(function(data) : Promise<void> {
    let assertion = data["assertion"];

    // Request all the interesting things for the proof
    let requests : Promise<any>[] = [];
    let requests_map = {
      thesis: push_and_get_index(requests, get_current_workset().do_api_request(`get_sentence/${assertion["thesis"]}`)),
      proof_tree: push_and_get_index(requests, get_current_workset().do_api_request(`get_proof_tree/${assertion["thesis"]}`)),
      ess_hyps_sent: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, get_current_workset().do_api_request(`get_sentence/${el}`)); }),
      float_hyps_sent: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, get_current_workset().do_api_request(`get_sentence/${el}`)); }),
      /*ess_hyps_ass: assertion["ess_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),
      float_hyps_ass: assertion["float_hyps"].map(function (el) { return push_and_get_index(requests, jsonAjax(`/api/1/workset/${workset.id}/get_assertion/${el}`)); }),*/
    };

    // Fire all the requests and then feed the results to the template
    return Promise.all(requests).then(function(responses : Array<any>) : void {
      let internal = render_proof(responses[requests_map["proof_tree"]]["proof_tree"], div_id, include_non_essentials);
      $(`#${div_id}`).html(Mustache.render(PROOF_TEMPL, {
        thesis: get_current_renderer().render_from_codes(responses[requests_map["thesis"]]["sentence"]),
        ess_hyps_sent: requests_map["ess_hyps_sent"].map(function(el) { return get_current_renderer().render_from_codes(responses[el]["sentence"]); }),
        float_hyps_sent: requests_map["float_hyps_sent"].map(function(el) { return get_current_renderer().render_from_codes(responses[el]["sentence"]); }),
        dists: lastize(assertion["dists"].map(function(el) { return { dist: get_current_renderer().render_from_codes([el[0]]) + ", " + get_current_renderer().render_from_codes([el[1]]) }; })),
        proof: internal[0],
      }));
      for (let fun of internal[1]) {
        fun();
      }
    });
  }).catch(catch_all);
}

const PROOF_TEMPL = `
  Floating hypotheses:<ol>
  {{ #float_hyps_sent }}
  <li>{{{ . }}}</li>
  {{ /float_hyps_sent }}
  </ol>
  Essential hypotheses:<ol>
  {{ #ess_hyps_sent }}
  <li>{{{ . }}}</li>
  {{ /ess_hyps_sent }}
  </ol>
  <p>Distinct variables: {{ #dists }}{{{ dist }}}{{ ^last }}; {{ /last }}{{ /dists }}</p>
  <p>Thesis: {{{ thesis }}}</p>
  <center><table summary="Proof of theorem" cellspacing="0" border="" bgcolor="#EEFFFA">
  <caption><b>Proof of theorem <b>{{ label }}</b></caption>
  <tbody>
  <tr><th>Step</th><th>Hyp</th><th>Ref</th><th>Expression</th><th>Dists</th></tr>
  {{{ proof }}}
  </tbody>
  </table></center>
`;

const PROOF_STEP_TEMPL = `
  {{ #children }}
  {{{ . }}}
  {{ /children }}
  <tr>
  <td>{{ step }}</td>
  <td>{{ #children_steps }}{{ step }}{{ ^last }}, {{ /last }}{{ /children_steps }}</td>
  <td>{{ #label_tok }}<a id="{{ udid }}" href="#">{{ /label_tok }}{{ label }}{{ #label_tok }}</a>{{ /label_tok }} <span class="r" style="color: {{ number_color }}">{{ number }}</span></td>
  <td><span class="i">{{{ indentation }}}</span> {{{ sentence }}}</td>
  <td>{{ #dists }}{{{ dist }}}{{ ^last }}; {{ /last }}{{ /dists }}</td>
  </tr>
`;
