# USAGE: from the examples directory:
# python2.7 run.py count_users_and_events 20150401 20150401

import collections

from pyaloha.base import DataAggregator as BaseDataAggregator
from pyaloha.base import DataStreamWorker as BaseDataStreamWorker
from pyaloha.base import StatsProcessor as BaseStatsProcessor
from pyaloha.protocol import day_serialize


USERS_AND_EVENTS_HEADER = '''\
Users & Events by days
Date\tUsers\tEvents
'''


def info_item():
    return {
        'users': set(),
        'events': 0
    }


class DataStreamWorker(BaseDataStreamWorker):
    def __init__(self):
        super(DataStreamWorker, self).__init__()

        self.dates = collections.defaultdict(info_item)

    def process_unspecified(self, event):
        dte = day_serialize(event.event_time.dtime.date())
        if event.event_time.is_accurate:
            self.dates[dte]['users'].add(event.user_info.uid)
            self.dates[dte]['events'] += 1


class DataAggregator(BaseDataAggregator):
    def __init__(self, *args, **kwargs):
        super(DataAggregator, self).__init__(
            *args, **kwargs
        )

        self.dates = collections.defaultdict(info_item)

    def aggregate(self, results):
        for dte, stats in results.dates.iteritems():
            self.dates[dte]['users'].update(stats['users'])
            self.dates[dte]['events'] += stats['events']


class StatsProcessor(BaseStatsProcessor):
    def gen_stats(self):
        yield USERS_AND_EVENTS_HEADER, (
            (dte, len(stats['users']), stats['events'])
            for dte, stats in sorted(
                self.aggregator.dates.iteritems()
            )
        )
