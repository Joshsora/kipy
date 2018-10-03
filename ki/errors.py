import textwrap


class UserError(Exception):
    def __init__(self, name, description=None, solution=None):
        self.name = name
        self.description = description
        self.solution = solution

    def __repr__(self):
        formatted_description = UserError.format_text_block(self.description)
        formatted_solution = UserError.format_text_block(self.solution)
        return 'Error name: %s\nDescription:\n%s\nSolution:\n%s' % (
            self.name, formatted_description, formatted_solution)

    @staticmethod
    def format_text_block(text_block):
        formatted_text_block = '| '
        lines = text_block.splitlines()
        for line in lines:
            formatted_text_block += '\n| '.join(textwrap.wrap(line)) + '\n| '
        return formatted_text_block[:-3]
