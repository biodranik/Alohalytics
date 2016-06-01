
import multiprocessing
import os
import shutil
import sys
import traceback

from pyaloha.event_factory import EventFactory
from pyaloha.protocol import WorkerResults, str2date
from pyaloha.worker import invoke_cmd_worker, load_plugin, setup_logs


class DataStreamWorker(object):
    __events__ = tuple()

    def __init__(self, event_factory=None):
        self._event_factory = event_factory or EventFactory(custom_events=[])

    def process_unspecified(self, event):
        pass

    def process_event(self,
                      key, event_time_p, user_info_p,
                      str_data_p, str_data_len):
        try:
            ev = self._event_factory.make_event(
                key, event_time_p[0], user_info_p[0],
                str_data_p, str_data_len
            )
            ev.process_me(self)
        except Exception:
            logger = multiprocessing.get_logger()
            logger.error(traceback.format_exc())

    def dumps_results(self):
        return WorkerResults.dumps_object(self, debug=False)

    def pre_output(self):
        pass


class DataAggregator(object):
    def __init__(self, results_dir=None, post_aggregate_worker=lambda x: x):
        self.post_aggregate_worker = post_aggregate_worker

        self.results_dir = results_dir
        if self.results_dir:
            self.created_dirs = set()
            shutil.rmtree(self.results_dir, ignore_errors=True)

    def aggregate(self):
        pass

    def post_aggregate(self, pool=None):
        pass


class StatsProcessor(object):
    def __init__(self, aggregator):
        self.aggregator = aggregator

    def process_stats(self):
        pass

    # look at print_stats for results format
    def gen_stats(self):
        raise NotImplementedError()

    def print_stats(self):
        for header, stats_generator in self.gen_stats():
            print '-' * 20
            print header
            print '-' * 20
            for row in stats_generator:
                print '\t'.join(map(unicode, row))
            print


def aggregate_raw_data(
        data_dir, results_dir, plugin_dir, plugin,
        start_date=None, end_date=None,
        worker_num=3 * multiprocessing.cpu_count() / 4):
    setup_logs()
    logger = multiprocessing.get_logger()
    pool = multiprocessing.Pool(worker_num)

    try:
        items = (
            (plugin_dir, plugin, os.path.join(data_dir, fname))
            for fname in os.listdir(data_dir)
            if check_fname(fname, start_date, end_date)
        )

        logger.info('Aggregator: aggregate')
        aggregator = load_plugin(
            plugin, plugin_dir=plugin_dir
        ).DataAggregator(results_dir)
        for results in pool.imap_unordered(invoke_cmd_worker, items):
            aggregator.aggregate(WorkerResults.loads_object(results))

        logger.info('Aggregator: post_aggregate')
        aggregator.post_aggregate(pool)
        logger.info('Aggregator: done')
    finally:
        pool.terminate()
        pool.join()

    return aggregator


def run(plugin_name, start_date, end_date, plugin_dir,
        data_dir='/mnt/disk1/alohalytics/by_date',
        results_dir='./stats'):
    aggregator = aggregate_raw_data(
        data_dir, results_dir, plugin_dir, plugin_name,
        start_date, end_date
    )
    stats = load_plugin(
        plugin_name, plugin_dir=plugin_dir
    ).StatsProcessor(aggregator)
    logger = multiprocessing.get_logger()
    logger.info('Stats: processing')
    stats.process_stats()
    logger.info('Stats: outputting')
    stats.print_stats()
    logger.info('Stats: done')


def check_fname(filename, start_date, end_date):
    if filename[0] == '.':
        return False
    return check_date(filename, start_date, end_date)


def check_date(filename, start_date, end_date):
    fdate = str2date(filename[-11:-3])
    if start_date and fdate < start_date:
        return False
    if end_date and fdate > end_date:
        return False
    return True


def cmd_run(plugin_dir):
    # TODO: argparse
    plugin_name = sys.argv[1]
    try:
        start_date, end_date = map(str2date, sys.argv[2:4])
    except ValueError:
        start_date, end_date = None, None

    run(plugin_name, start_date, end_date, plugin_dir=plugin_dir)
