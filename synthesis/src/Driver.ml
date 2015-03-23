
open JITD

let source_files:string list ref = ref [];;

let arg_spec = Arg.align [ 
  ( "-ignored", Arg.Unit(fun () -> print_endline "this option is ignored"), 
    "This option is ignored"
  )
];;

Arg.parse 
  arg_spec 
  (fun f -> source_files := f :: !source_files)
	"jitd [options] files";;

let prog = 
  (List.fold_left merge_programs empty_program
    (List.map 
      (fun f -> 
        let buf = (Lexing.from_channel (open_in f)) in
          try 
            JITDParser.program JITDLexer.token buf
          with 
            | JITD.ParseError(msg, pos) ->
                print_endline ("Syntax Error ("^
                                f^":"^
                                (string_of_int pos.Lexing.pos_lnum)^" '"^
                                (Lexing.lexeme buf)^"'): "^msg);
                exit (-1)
      )
      (List.rev !source_files) 
    )
  )
;;


let policies = 
  try 
    List.map (Optimizer.optimize_policy prog) prog.JITD.policies;
  with 
    | Optimizer.StmtError(msg, stmt) -> (
      print_endline (msg^": ");
      print_endline (JITD.string_of_stmt ~prefix:"  " stmt);
      exit (-1)
    )
    | Optimizer.ExprError(msg, stmt) -> (
      print_endline (msg^": "^(JITD.string_of_expr stmt));
      exit (-1)
    )
  
;;

List.iter print_endline (List.map string_of_policy policies)
