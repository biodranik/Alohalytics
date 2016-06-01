import importlib
import logging
import multiprocessing
import os
import subprocess
import sys
import traceback

from pyaloha.ccode import iterate_events


def setup_logs(filepath=os.path.join('/tmp', 'py_alohalytics_stats.log')):
    logger = multiprocessing.get_logger()

    handler = logging.FileHandler(filepath)
    handler.setFormatter(logging.Formatter(fmt='%(asctime)-15s %(message)s'))
    logger.addHandler(handler)

    logger.setLevel(logging.INFO)


def load_plugin(plugin_name, plugin_dir):
    sys.path.append(plugin_dir)
    return importlib.import_module(plugin_name)


def invoke_cmd_worker(item):
    try:
        logger = multiprocessing.get_logger()
        pid = multiprocessing.current_process().pid

        plugin_dir, plugin, filepath = item
        worker_fpath = os.path.abspath(__file__)
        cmd = 'gzip -d -c %s | python2.7 %s %s %s' % (
            filepath, worker_fpath, plugin_dir, plugin
        )
        logger.info(
            '%d: Starting job: %s', pid, cmd
        )

        env = os.environ.copy()
        env['PYTHONPATH'] = os.pathsep.join(sys.path)

        process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, shell=True,
            env=env
        )
        output = process.communicate()[0]
        return output
    except Exception as e:
        traceback.print_exc(e)


def worker():
    setup_logs()
    logger = multiprocessing.get_logger()

    try:
        plugin_dir = sys.argv[1]
        plugin = sys.argv[2]
        processor = load_plugin(
            plugin, plugin_dir=plugin_dir
        ).DataStreamWorker()
        iterate_events(processor)
        processor.pre_output()
        print processor.dumps_results()
    except Exception:
        logger.error(traceback.format_exc())

if __name__ == '__main__':
    worker()
