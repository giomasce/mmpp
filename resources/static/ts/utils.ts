/// <reference path="jquery.d.ts"/>
/// <reference path="mustache.d.ts"/>

export function jsonAjax(url : string, dump : boolean = true, dump_content : boolean = true, async : boolean = true) : JQueryPromise<any> {
  if (dump) {
    $("#console_text").append("\n\nRequesting: " + url);
  }
  return $.ajax({
    url: url,
    dataType: "json",
    async: async,
  }).then(function(data) {
    if (dump) {
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

export function lastize(list : object[]) : object[] {
  if (list.length !== 0) {
    list[list.length-1]["last"] = true;
  }
  return list;
}

export function invert_list(list : string[]) : { [id: string] : number } {
  let ret : { [id: string] : number } = {};
  for (let i : number = 0; i < list.length; i++) {
    ret[list[i]] = i;
  }
  return ret;
}

export function dump_to_console(log : string) {
  $("#console_text").append("\n" + log);
}

export function gen_random_id() : string {
  return Math.floor(Math.random() * 0x1000000).toString(16);
}

let last_serial : number = 0;
export function get_serial() : string {
  last_serial += 1;
  return last_serial.toString();
}

/* Implementation of the the function spectrumToRGB() in the official metamath;
   the color codes are copied from there, under the label "L53 empirical". */
const COLORS : number[][] = [
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

export function spectrum_to_rgb(color : number, max_color : number) : string {
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

export function push_and_get_index<T>(array : T[], object : T) : number {
  let index : number = array.length;
  array.push(object);
  return index;
}

export function serialize(array : (callback : (bool)=>void)=>void[]) : void {
  
}
