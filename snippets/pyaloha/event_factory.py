import itertools

from pyaloha.event import Event


class EventFactory(object):
    def __init__(self, custom_events):
        self.check_events(custom_events)

        self.registered = dict(
            (key, event_cls)
            for event_cls in custom_events
            for key in event_cls.keys
        )

    def make_event(self, key, *args, **kwargs):
        return self.registered.get(key, Event)(key, *args, **kwargs)

    @classmethod
    def check_events(cls, custom_events):
        event_keys = list(itertools.chain.from_iterable(
            event_cls.keys for event_cls in custom_events
        ))
        if len(frozenset(event_keys)) != len(event_keys):
            raise ImportError('Intersection of keys in events')
