// CodeMirror, copyright (c) by Marijn Haverbeke and others
// Distributed under an MIT license: https://codemirror.net/LICENSE

// Adapted by Fergus Baker for Cadabra 2 (c) Kasper Peeters

(function(mod) {
  if (typeof exports == "object" && typeof module == "object") // CommonJS
    mod(require("../../lib/codemirror"));
  else if (typeof define == "function" && define.amd) // AMD
    define(["../../lib/codemirror"], mod);
  else // Plain browser env
    mod(CodeMirror);
})(function(CodeMirror) {
  "use strict";

  // lexing starts here!
  CodeMirror.defineMode('cadabra', function(cfg, cfg_parser) {
    // use the python code mirror mode
    var pythonmode = CodeMirror.getMode(cfg, {name:'python'});

    function isWhitespace(char) {
      return !(char) || char == ' ' || char == '\n' || char == '\t'
    }

    function eatWord(stream) {
      var count = 0;
      while(stream.eat(/[\w\d]+/g)){
        // eat word
        count++;
      }
      return count;
    }

    function cadabraToken(stream, state) {
      // big long basic if/else parser

      // get current token string
      var current = stream.current();

      // ignore if string; use lookbehind to see if escaped
      if (current.match(/^(?<!\\)".*(?<!\\)"/g)) {
        return 'python';
      }

      if (current.match(/#/)) {
        // lines containing '::' or ':=' must be escaped
        // controlled by the state.allow_comments
        if (state.allow_comments && (! current.match(/(::|:=)/g) || stream.indentation() == stream.column() )) {
          // python comment
          return 'python';
        } else {
          // escape python comments
          stream.backUp(current.length-1);
          return "operator";
        }
      }
      else if (current.match(/\?/g)) {
        return 'operator';
      }
      else if (current.match(/:/g)) {
        // don't indent; have to update scopes

        if (stream.peek() == ':') {
          // handle property assign operator
          state.lastToken="::"
          state.scopes.pop();

          // modify state for cadabra comments
          state.allow_comments = false;

          // consume second :
          stream.next();
          // eat property
          eatWord(stream);
          return 'builtin';
        } else if (stream.peek() == '=') {
          // handle walrus assign operator
          state.lastToken=":="
          state.scopes.pop();

          // modify state for cadabra comments
          state.allow_comments = false;

          // consume = 
          stream.next();
          return 'operator';

        } else return 'python';
      }
      else if (current.match(/[.;]$/gm)) {
        if ( isWhitespace(stream.peek()) ) {
          return 'operator';
        }
      }
      else if (current.match(/_/g)) {
        // handle lowering indices operator
        // ensure stream not in variable
        if (current.length > 1) {
          stream.backUp(1);
          return null;
        }
        if (stream.peek() == '{') {
          return 'operator'
        }
      }
      else if (current.match(/\$/g)) {
        // cadabra lambdas
        return 'def';
      }
      else if (current.match(/\\/)) {
        var count = eatWord(stream);
        current = stream.current();
        if (current.match(/(lambda|int)/g)) {
          return null;
        } else {
          stream.backUp(count);
          return 'python';
        }

      }
      else {
        // leave alone
        return 'python';
      }
    }

    // global multiline match triggers; this could probably be done more
    // efficiently with doing explicit checks for positioning, but this code
    // doesn't need to run particularly fast, so for convenience
    // grouping together
    const triggers = /[\?:#_;.$\\]/gm

    return {
      startState: () => {
        // init python mode, cadabra mode adds allow_comments to state
        var init_state = pythonmode.startState();
        init_state.allow_comments = true;
        return init_state;
      },
      token: (stream, state) => {
        // clear stored cadabra state
        if (stream.sol()) {
          state.allow_comments = true;
        }

        var ret = pythonmode.token(stream, state);

        // cadabra parse if triggered
        if (stream.current().match(triggers)) {
          var cdb_ret = cadabraToken(stream, state);
          ret = cdb_ret !== 'python' ? cdb_ret : ret;
        }
        return ret;
      },
      indent: (state, textAfter) => {
        // so that properties don't indent oddly

        if (state.lastToken == '::') return 0;

        // let python handle indents
        return pythonmode.indent(state, textAfter);
      }
    }
  });
    CodeMirror.defineMIME("text/cadabra2", "cadabra");
});
