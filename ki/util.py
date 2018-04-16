class AllocationError(Exception):
    pass


class IDAllocator(object):
    def __init__(self, min_id, max_id):
        self.next_id = min_id
        self.max_id = max_id
        self.unused_ids = set()

    def allocate(self):
        if self.unused_ids:
            return self.unused_ids.pop()
        if self.next_id > self.max_id:
            raise AllocationError('ID allocation limit reached -- %d' % self.max_id)
        allocated_id = self.next_id
        self.next_id += 1
        return allocated_id

    def free(self, id):
        self.unused_ids.add(id)
