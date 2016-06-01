import itertools
import os

from pyaloha.base import DataAggregator as BaseDataAggregator
from pyaloha.base import StatsProcessor as BaseStatsProcessor

from pyaloha.protocol import FileProtocol, day_deserialize, str2date


class DataAggregator(BaseDataAggregator):
    def __init__(self, *args, **kwargs):
        super(DataAggregator, self).__init__(
            post_aggregate_worker=post_aggregate_worker,
            *args, **kwargs
        )

    def aggregate(self, processor_results):
        for dte, data_dict in processor_results.data_per_days.iteritems():
            self.save_day(
                fname=self.get_result_file_path(day_deserialize(dte)),
                iterable=data_dict.iteritems(),
                mode='a+'
            )

    def post_aggregate(self, pool=None):
        engine = itertools.imap
        if pool:
            engine = pool.imap_unordered

        for _ in engine(
                self.post_aggregate_worker,
                self.iterate_saved_days()):
            pass

    def get_result_file_path(self, dte):
        if not self.results_dir:
            raise Exception('Results dir is not set to use this method')

        month_dir = os.path.join(
            self.results_dir, str(dte.year), str(dte.month)
        )
        if month_dir not in self.created_dirs:
            try:
                os.makedirs(month_dir)
                self.created_dirs.add(month_dir)
            except OSError:
                pass
        return os.path.join(month_dir, str(dte.day))

    def extract_date_from_path(self, path):
        dte = ''.join(map(
            lambda s: s.zfill(2),
            path.split('/')[-3:])
        )
        return str2date(dte)

    @staticmethod
    def save_day(iterable, fname=None, mode='w'):
        with open(fname, mode) as fout:
            for obj in iterable:
                fout.write(FileProtocol.dumps(obj) + '\n')

    @staticmethod
    def load_day(fname):
        with open(fname) as fin:
            for line in fin:
                yield FileProtocol.loads(line)

    def iterate_saved_days(self):
        for root, dirs, fnames in os.walk(self.results_dir):
            for fn in fnames:
                yield os.path.join(root, fn)


def post_aggregate_worker(fname):
    results = DataAggregator.load_day(fname)
    # TODO: agg
    reduced = dict(results)
    DataAggregator.save_day(
        fname=fname,
        iterable=reduced.iteritems()
    )


class StatsSubscriber(object):
    def collect(self, item):
        raise NotImplementedError()

    def gen_stats(self):
        raise NotImplementedError()


class StatsProcessor(BaseStatsProcessor):
    subscribers = tuple()

    def prepare_collectable_item(self, fname):
        data_list = self.aggregator.load_day(fname)
        return self.aggregator.extract_date_from_path(fname), data_list

    def processable_iterator(self):
        return sorted(
            itertools.imap(
                self.prepare_collectable_item,
                self.aggregator.iterate_saved_days()
            )
        )

    def process_stats(self):
        self.procs = tuple(s() for s in self.subscribers)

        for dte, data_generator in self.processable_iterator():
            data_tuple = tuple(data_generator)
            for p in self.procs:
                p.collect((dte, data_tuple))

    def gen_stats(self):
        for p in self.procs:
            yield p.gen_stats()
