
# before :
class Node:
    def __init__(self, value: int, next_node: 'Node' = None):
        self.value = value
        self.next_node = next_node


# after :
from __future__ import annotations
class Node:
    def __init__(self, value: int, next_node: Node | None = None):
        self.value = value
        self.next_node = next_node
