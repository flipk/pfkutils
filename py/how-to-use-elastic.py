
def main():
    elastic = ElasticIF.ElasticIF(ELASTIC_IPADDR,
                                  ELASTIC_USERNAME, ELASTIC_PASSWORD,
                                  CERT_FINGERPRINT)
    bulk = self._elastic.bulk_start(self._index_name, self._bulk_insert_size)
    for loop:
        bulk.insert(record):
    bulk.finish()

