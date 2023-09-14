from collections import namedtuple
from inspect import signature

ReductionResult = namedtuple("ReductionResult", [
    "tree",
    "changed",
    "appearance_changed"
])

class Symbol:
    def __init__(self, sym):
        self.symbol = sym
    
    def show(self, *_, **__):
        return self.symbol

    def reduce(self):
        return ReductionResult(None, False, False)

    def apply(self, *__):
        return None, False

class Combinator:
    def __init__(self, name, function, have=[]):
        self.name = name
        self.function = function
        self.required = len(signature(function).parameters)
        self.have = have 

    def show(self, bracket=False):
        args = "".join(
            n.show(bracket=True) for n in self.have
        )
        res = self.name + args
        if bracket and len(self.have) > 0:
            res = f"({res})"
        return res

    def reduce(self):
        return ReductionResult(None, False, False)

    def apply(self, arg):
        new_have = self.have + [arg]
        if len(new_have) == self.required:
            raw_result = self.function(*new_have)
            as_tree    = Combinator.treeify(raw_result)
            return as_tree, True
        new = Combinator(self.name, self.function, new_have)
        return new, False

    @staticmethod
    def treeify(arg):
        if type(arg) != tuple:
            return arg
        assert len(arg) == 2
        lhs, rhs = arg
        lhs = Combinator.treeify(lhs)
        rhs = Combinator.treeify(rhs)
        return Tree(lhs, rhs)

class Tree:
    def __init__(self, l, r):
        self.left  = l
        self.right = r

    def show(self, bracket=False):
        s = self.left.show(False) + self.right.show(True)
        return f"({s})" if bracket else s

    def reduce(self):
        lhs = self.left.reduce()
        if lhs.changed:
            t = Tree(lhs.tree, self.right)
            return ReductionResult(t, True, lhs.appearance_changed)

        rhs = self.right.reduce()
        if rhs.changed:
            t = Tree(self.left, rhs.tree)
            return ReductionResult(t, True, rhs.appearance_changed)

        res, appearance = self.left.apply(self.right)
        if res != None:
            return ReductionResult(res, True, appearance)
        return ReductionResult(self, False, False)
        
    def apply(self, arg):
        return None, False

def full_reduce_tree(tree):
    steps = [tree]
    while True:
        res = tree.reduce()
        if not res.changed:
            break
        if res.appearance_changed:
            steps.append(res.tree)
        tree = res.tree
    return steps, tree
