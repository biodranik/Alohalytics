import datetime
import inspect
import json

DAY_FORMAT = '%Y%m%d'
UNIQ_DAY_FORMAT = 'dt' + DAY_FORMAT


def day_serialize(dtime):
    return dtime.strftime(UNIQ_DAY_FORMAT)


def day_deserialize(s):
    return datetime.datetime.strptime(s, UNIQ_DAY_FORMAT)


def str2date(s):
    return datetime.datetime.strptime(s, DAY_FORMAT).date()


def custom_loads(dct):
    if '__stype__' in dct and dct['__stype__'] == 'repr':
        return eval(dct['__svalue__'])

    for str_key in dct.keys():
        try:
            int_key = int(str_key)
        except ValueError:
            break
        else:
            dct[int_key] = dct[str_key]
            del dct[str_key]

    return dct


class CustomEncoder(json.JSONEncoder):
    def default(self, obj):
        if hasattr(obj, '__dumpdict__'):
            return obj.__dumpdict__()

        if isinstance(obj, (datetime.datetime, datetime.date, set, frozenset)):
            return {'__stype__': 'repr', '__svalue__': repr(obj)}
        # Let the base class default method raise the TypeError
        return json.JSONEncoder.default(self, obj)


class FileProtocol(object):
    @classmethod
    def dumps(cls, obj, debug=False):
        return json.dumps(
            obj, cls=CustomEncoder
        )

    @classmethod
    def loads(cls, json_text):
        return json.loads(json_text, object_hook=custom_loads)


class WorkerResults(FileProtocol):
    @classmethod
    def dumps_object(cls, obj, debug=False):
        '''
        NOTE: Please, check the keys of your dicts to be basic types only
        '''
        return cls.dumps(
            [
                pair
                for pair in inspect.getmembers(
                    obj, lambda m: not callable(m)
                )
                if not pair[0].startswith('_')
            ]
        )

    @classmethod
    def loads_object(cls, json_text):
        instance = WorkerResults()
        for key, value in cls.loads(json_text):
            setattr(instance, key, value)
        return instance
