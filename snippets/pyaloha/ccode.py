import ctypes
import datetime
import itertools
import os


ctypes.set_conversion_mode('utf8', 'strict')


def c_unicode(c_data):
    return unicode(ctypes.string_at(c_data), 'utf8')


def c_string(c_data):
    return c_data


class CEVENTTIME(ctypes.Structure):
    _fields_ = [
        ('client_created', ctypes.c_uint64),
        ('server_upload', ctypes.c_uint64)
    ]

    dtime = None
    is_accurate = False

    current_time = datetime.datetime.utcnow()
    delta_past = datetime.timedelta(days=6 * 30)
    delta_future = datetime.timedelta(days=1)

    def _setup_time(self):
        self.client_dtime = datetime.datetime.utcfromtimestamp(
            self.client_created / 1000.  # timestamp is in millisecs
        )
        self.server_dtime = datetime.datetime.utcfromtimestamp(
            self.server_upload / 1000.  # timestamp is in millisecs
        )

        self.dtime = self.server_dtime
        if self.client_dtime >= self.server_dtime - self.delta_past and\
                self.client_dtime <= self.server_dtime + self.delta_future:
            self.dtime = self.client_dtime

            self.is_accurate = True

    def get_approx_time(self):
        if self.dtime is None:
            self._setup_time()
        return self.dtime, self.accuracy


class IDInfo(ctypes.Structure):
    _fields_ = [
        ('os_t', ctypes.c_byte),
    ]

    uid = None

    def __dumpdict__(self):
        return {
            'os_t': self.os_t
        }


class GeoIDInfo(IDInfo):
    _fields_ = [
        ('lat', ctypes.c_float),
        ('lon', ctypes.c_float)
    ]

    def has_geo(self):
        return (
            round(self.lat, 2) != 0.0 or
            round(self.lon, 2) != 0.0
        )

    def get_location(self):
        if self.has_geo():
            return (self.lat, self.lon)
        return None

    def __dumpdict__(self):
        dct = super(GeoIDInfo, self).__dumpdict__()
        if self.has_geo():
            dct['lat'] = round(self.lat, 6)
            dct['lon'] = round(self.lon, 6)
        return dct


class CUSERINFO(GeoIDInfo):
    _fields_ = [
        ('raw_uid', (ctypes.c_char * 32)),
    ]

    def setup(self):
        setattr(self, 'uid', int(self.raw_uid, 16))

    def stripped(self):
        if self.has_geo():
            return GeoIDInfo(
                uid=self.uid, os_t=self.os_t,
                lat=self.lat, lon=self.lon
            )
        return IDInfo(
            uid=self.uid, os_t=self.os_t
        )


CCALLBACK = ctypes.CFUNCTYPE(
    None,
    ctypes.c_char_p,
    ctypes.POINTER(CEVENTTIME),
    ctypes.POINTER(CUSERINFO),
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.c_int
)  # key, event_time, user_info, data, data_len


def iterate_events(stream_processor):
    base_path = os.path.dirname(os.path.abspath(__file__))
    c_module = ctypes.cdll.LoadLibrary(
        os.path.join(
            base_path, '..',
            'c_api', 'build', 'iterate_events.so'
        )
    )
    use_keys = tuple(itertools.chain.from_iterable(
        e.keys for e in stream_processor.__events__
    ))
    keylist_type = ctypes.c_char_p * len(use_keys)
    c_module.iterate.argtypes = [CCALLBACK, keylist_type, ctypes.c_int]
    c_module.iterate(
        CCALLBACK(
            stream_processor.process_event
        ),
        keylist_type(*use_keys),
        len(use_keys)
    )
