
import sys
import collections.abc
from elasticsearch import Elasticsearch
import threading


ELASTIC_BULK_INSERT_SIZE = 400
# ELASTIC_INDEX_MAPPINGS = {
#     "properties": {
#         "item_counter": {
#             "type": "long"
#         },
#         "some_int": {
#             "type": "long"
#         },
#         "Date": {
#             "type": "date",
#             "format": "MM/dd/yyyy HH:mm:ss"
#         }
#     }
# }


class BulkOps:
    """
    an accumulator class for elastic search bulk operations.
    use ElasticIF.bulk_start() to get one of these.
    """
    _bulk_index: str
    _bulk_ops_size: int   # max size of ops
    _lock: threading.Lock
    _eif_lock: threading.Lock
    _bulk_ops: list
    _client: Elasticsearch

    def __init__(self,
                 client: Elasticsearch,
                 eiflock: threading.Lock,
                 index_name: str,
                 bulk_size: int = ELASTIC_BULK_INSERT_SIZE):
        self._bulk_index = index_name
        self._bulk_ops_size = bulk_size
        self._lock = threading.Lock()
        self._eif_lock = eiflock
        self._bulk_ops = []
        self._client = client

    def insert(self, doc: dict) -> bool:
        self._lock.acquire()
        self._bulk_ops.append({"index": {"_index": self._bulk_index}})
        self._bulk_ops.append(doc)
        if len(self._bulk_ops) >= self._bulk_ops_size:
            success = self.finish(do_lock=False)
        else:
            success = True
        self._lock.release()
        return success

    def update(self, _id: str, doc: dict) -> bool:
        self._lock.acquire()
        self._bulk_ops.append({"update": {"_index": self._bulk_index, "_id": _id}})
        self._bulk_ops.append({"doc": doc})
        if len(self._bulk_ops) >= self._bulk_ops_size:
            success = self.finish(do_lock=False)
        else:
            success = True
        self._lock.release()
        return success

    def delete(self, _id: str) -> bool:
        self._lock.acquire()
        self._bulk_ops.append({"delete": {"_index": self._bulk_index, "_id": _id}})
        if len(self._bulk_ops) >= self._bulk_ops_size:
            success = self.finish(do_lock=False)
        else:
            success = True
        self._lock.release()
        return success

    def finish(self, do_lock: bool = True, refresh: bool = False) -> None:
        errs = []
        success = True
        if do_lock:
            self._lock.acquire()
        if len(self._bulk_ops) > 0:
            self._eif_lock.acquire()
            try:
                # print(f'performing bulk with {len(self._bulk_ops)} ops')
                resp = self._client.bulk(operations=self._bulk_ops,
                                         refresh=refresh)
                success = True
            except BaseException as e:
                resp = None
                success = False
                print("client bulk exception:", e)
            self._eif_lock.release()
            if success:
                # print("bulk response:", resp)
                if resp.get('errors', None):
                    i: dict
                    for i in resp.get('items', []):
                        for j, k in i.items():
                            status = k.get('status', 499)
                            if 400 <= status <= 499:
                                errs.append({j: k})
                    success = False
            self._bulk_ops = []
        if do_lock:
            self._lock.release()
        if len(errs) > 0:
            print("errors:", errs)
        return success, errs

    def __del__(self):
        if len(self._bulk_ops) > 0:
            print("\n *** \n"
                  " *** WARNING: ElasticIF _BulkOps destroyed with operations pending!\n"
                  " ***          Did you forget to call flush?\n"
                  " *** \n\n")
            # this may not work, but try it
            self.finish()


class ElasticIF:
    """
    a class for interfacing to ElasticSearch.
    """
    _client: Elasticsearch
    _lock: threading.Lock

    def __init__(self,
                 ipaddr: str,
                 port: int,
                 username: str,
                 password: str,
                 cert_fingerprint: str):
        try:
            url = f'https://{ipaddr}:{port}'
            print(f'attempting connection to url: {url}')
            self._client = Elasticsearch(
                url,
                ssl_assert_fingerprint=cert_fingerprint,
                basic_auth=(username, password)
            )
            print("connection success:", self._client.info())
        except BaseException as err:
            print("ElasticIF detected error: ", err)
            print("    of type: ", type(err))
            exit(1)
        self._lock = threading.Lock()

    def get_records(self, index_name: str,
                    source: bool = False,
                    from_: int = 0,
                    size: int = 10,
                    body: dict = None) -> (bool, dict):
        """
        perform a query of records from ElasticSearch.
        :param index_name: the name of the index.
        :param source: bool: return the entire source record or not.
        :param from_: int: starting record number to retrieve, default 0.
        :param size: int: number of records to retrieve, default 10.
        :param body: dict: query language, default to match all.
        :return: (success: bool, response: dict)
        """
        if not body:
            body = {'query': {'match_all': {}}}
        success = False
        resp = None
        self._lock.acquire()
        try:
            resp = self._client.search(
                index=index_name,
                source=source,
                size=size, from_=from_,
                body=body
            )
            success = True
        except BaseException as e:
            print("query: ", body, "\nexception:", e)
        self._lock.release()
        return success, resp

    def record_generator(self, index_name: str, source=False,
                         size=10, body=None, scroll=None) -> collections.abc.Iterable:
        if not body:
            body = {'query': {'match_all': {}}}
        if not scroll:
            scroll = '5m'
        try:
            self._lock.acquire()
            resp = self._client.search(index=index_name, source=source,
                                       size=size, body=body, scroll=scroll)
            self._lock.release()
            scroll_id = resp['_scroll_id']
            hits = resp['hits']['hits']
            # print("search got %u hits" % len(hits))
            for h in hits:
                yield h
            while len(hits) > 0:
                self._lock.acquire()
                resp = self._client.scroll(scroll_id=scroll_id, scroll=scroll)
                self._lock.release()
                hits = resp['hits']['hits']
                # print("scroll got %u hits" % len(hits))
                for h in hits:
                    yield h
        # NOTE the normal behavior of a generator is to throw
        # a GeneratorExit exception when the generator is complete.
        # GeneratorExit is derived from BaseExeption, but not Exception,
        # so if we catch BaseException, we must ignore GeneratorExit.
        # if we only catch Exception, we don't have to worry about that.
        #   except BaseException as e:
        #         if not isinstance(e, GeneratorExit):
        #             print("exception:", e)
        #             raise
        except Exception as e:
            print("exception:", e)
            raise

    def update_record(self, index_name: str, _id, _doc: dict,
                      refresh: str = 'false') -> (bool, dict):
        resp = None
        success: bool = False
        try:
            self._lock.acquire()
            resp = self._client.update(
                index=index_name,
                id=_id,
                refresh=refresh,
                doc=_doc
                )
            self._lock.release()
            success = True
        except BaseException as e:
            print("update exception:", e)
        return success, resp

    def delete_record(self, index_name: str, _id,
                      refresh: str = 'false') -> (bool, dict):
        resp = None
        success = True
        try:
            self._lock.acquire()
            resp = self._client.delete(
                index=index_name,
                refresh=refresh,
                id=_id
                )
            self._lock.release()
            success = True
        except BaseException as e:
            print("delete id", _id, "exception:", e)
        return success, resp

    def insert_record(self, index_name: str, doc: dict,
                      refresh: bool = False) -> (bool, dict):
        success: bool = False
        resp = ''
        try:
            self._lock.acquire()
            resp = self._client.index(index=index_name,
                                      refresh=refresh,
                                      document=doc)
            self._lock.release()
            success = True
        except BaseException as e:
            print("client insert exception:", e)
        return success, resp

    def bulk_start(self, index_name: str,
                   bulk_size: int = ELASTIC_BULK_INSERT_SIZE) -> BulkOps:
        return BulkOps(self._client, self._lock, index_name, bulk_size)

    def create_index(self, index_name: str, mappings):
        success = False
        resp = ''
        try:
            self._lock.acquire()
            resp = self._client.indices.create(index=index_name,
                                               mappings=mappings)
            self._lock.release()
            success = True
        except BaseException as e:
            print("create index", index_name, "exception:", e)
        if success:
            print("NOTE: CREATED INDEX", index_name)
        return success, resp

    def delete_index(self, index_name: str):
        success = False
        resp = ''
        try:
            self._lock.acquire()
            resp = self._client.indices.delete(index=index_name)
            self._lock.release()
            success = True
        except BaseException as e:
            print("delete index", index_name, "exception:", e)
        if success:
            print("NOTE: DELETED INDEX", index_name)
        return success, resp
