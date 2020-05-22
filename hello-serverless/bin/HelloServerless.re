open Opium.Std;

let get_by_key =
  get("/:key", req => {
    let key = param(req, "key");
    `String("Hello " ++ key) |> respond';
  });

App.empty
  |> get_by_key
  |> App.run_command;
