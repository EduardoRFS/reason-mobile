include Map.Make(String);

open Helper;

// TODO: remove all to_seq |> of_seq
let of_list = l => l |> List.fold_left((m, (k, v)) => add(k, v, m), empty);

let to_yojson = (f, map) =>
  map |> bindings |> [%to_yojson: list((string, 'a))](f);
let of_yojson = (f, json: Yojson.Safe.t) => {
  let.ok content =
    switch (json) {
    | `Assoc(content) => Ok(content)
    | _ => Error("is not a object")
    };

  let.ok content =
    content
    |> List.fold_left(
         (acc, (key, value)) => {
           let.ok acc = acc;
           let.ok value = f(value);
           Ok([(key, value), ...acc]);
         },
         Ok([]),
       );

  Ok(of_list(content));
};

[@deriving yojson]
type x = string;
