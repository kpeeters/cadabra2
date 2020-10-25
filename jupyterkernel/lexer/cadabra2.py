""" very basic implementation of a pygments lexer for Cadabra2 """

from pygments.lexers import Python3Lexer
import pygments.token as token

__all__ = ["CadabraLexer"]

CADABRA_ROOT = [
    (r"::[\w\d]*", token.Keyword), # property attaches
    (r"\S#}", token.Text), # some comment exceptions
    (r"[;.]\s*$", token.Operator) # ; . operators
] + Python3Lexer.tokens['root']

class CadabraLexer(Python3Lexer):
    name="Cadabra",
    aliases=["cadabra","cadabra2"]
    filenames="*.cdb"

    tokens = {
        **Python3Lexer.tokens,
        **{'root':CADABRA_ROOT}
    }
