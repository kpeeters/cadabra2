
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : cadabra2.scm
;; DESCRIPTION : Initialize cadabra2 plugin
;; COPYRIGHT   : (C) 2025 Kasper Peeters
;;
;; This software falls under the GNU general public license and comes WITHOUT
;; ANY WARRANTY WHATSOEVER. See the file $TEXMACS_PATH/LICENSE for details.
;; If you don't have this file, write to the Free Software Foundation, Inc.,
;; 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; As in the python plugin, we need a way to make TeXmacs send
;; multi-line input without stripping newlines.

(define (cadabra-serialize lan t)
  (with u (pre-serialize lan t)
    (with s (texmacs->code (stree->tree u) "SourceCode")
	  (string-append  s  "\n<EOF>\n"))))

(plugin-configure cadabra
  (:require (url-exists-in-path? "cadabra2"))
  (:launch "cadabra2 --texmacs")
  (:serializer ,cadabra-serialize)
  (:session "Cadabra2"))
