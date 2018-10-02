class UserError(Exception):
    def __init__(self, name, description=None, solution=None):
        self.name = name
        self.description = description
        self.solution = solution
