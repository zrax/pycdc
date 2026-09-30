"""Tree & graph traversal scenarios: BST insert/search (recursion), tree
in-order traversal, BFS/DFS with explicit stack, graph BFS (deque), graph
DFS via nested recursion.

Observation targets (Python 3.13/3.14 bytecode):
 - nested functions (MAKE_FUNCTION closures) called inside loops
 - while-loop worklist with list.pop(0)/pop() duality
 - recursion + attribute access (`.` chains) inside conditions
 - deque append/popleft loop pattern (graph BFS)
 - nested def inside for + recursive self-reference
"""
from collections import deque


class TreeNode:
    def __init__(self, value):
        self.value = value
        self.left = None
        self.right = None


def insert(root, value):
    if root is None:
        return TreeNode(value)
    if value < root.value:
        root.left = insert(root.left, value)
    else:
        root.right = insert(root.right, value)
    return root


def build_tree(values):
    root = None
    for v in values:
        root = insert(root, v)
    return root


def inorder(root):
    if root is None:
        return []
    return inorder(root.left) + [root.value] + inorder(root.right)


def tree_height(root):
    if root is None:
        return 0
    return 1 + max(tree_height(root.left), tree_height(root.right))


def bfs(root):
    out = []
    queue = [root]
    while queue:
        node = queue.pop(0)
        if node is None:
            continue
        out.append(node.value)
        queue.append(node.left)
        queue.append(node.right)
    return out


def dfs_preorder(root):
    out = []
    stack = [root]
    while stack:
        node = stack.pop()
        if node is None:
            continue
        out.append(node.value)
        stack.append(node.right)
        stack.append(node.left)
    return out


def graph_bfs(graph, start):
    seen = {start}
    queue = deque([start])
    order = []
    while queue:
        node = queue.popleft()
        order.append(node)
        for nxt in graph[node]:
            if nxt not in seen:
                seen.add(nxt)
                queue.append(nxt)
    return order


def graph_dfs(graph, node, seen=None, order=None):
    if seen is None:
        seen = set()
        order = []
    if node in seen:
        return order
    seen.add(node)
    order.append(node)
    for nxt in graph[node]:
        graph_dfs(graph, nxt, seen, order)
    return order


def __test__():
    root = build_tree([5, 3, 8, 1, 4, 7, 9, 2, 6])
    graph = {1: [2, 3], 2: [4], 3: [4, 5], 4: [], 5: [1]}
    return [
        repr(inorder(root)),
        repr(tree_height(root)),
        repr(bfs(root)),
        repr(dfs_preorder(root)),
        repr(graph_bfs(graph, 1)),
        repr(graph_dfs(graph, 3)),
    ]
