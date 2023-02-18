def _latex_post_parser(text):
    return (
        text.replace("\\begin{dmath*}", "$")
        .replace("\\end{dmath*}", "$")
        .replace("\\discretionary{}{}{}", "")
        .replace("~", "")
    )


class Server:
    def __init__(self, kernel_instance):
        self._kernel = kernel_instance

    def send(self, data, typestr, parent_id, last_in_sequence):
        if typestr == "latex_view":
            data = _latex_post_parser(data)
            self._kernel._send_result(data)
        elif typestr == "image_png":
            self._kernel._send_image(data)
        elif typestr == "verbatim":
            self._kernel._send_code(data)
        elif typestr == "input_form":
            #  pass
            ...
        else:
            raise Exception("Unknown typestr '{}'".format(typestr))
        return 0

    def architecture(self):
        return "jupyter-kernel"

    def test(self):
        self._kernel._send_result("Test: We've gone on holiday by mistake!")

    def handles(self, otype):
        if otype == "latex_view" or otype == "image_png" or otype == "verbatim":
            return True
        return False

    def totals(self):
        # what does this do?
        return -1
