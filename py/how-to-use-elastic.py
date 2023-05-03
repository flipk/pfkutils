
def main():
    elastic = ElasticIF.ElasticIF(ELASTIC_IPADDR,
                                  ELASTIC_USERNAME, ELASTIC_PASSWORD,
                                  CERT_FINGERPRINT)
    bulk = self._elastic.bulk_start(self._index_name, self._bulk_insert_size)
    for loop:
        bulk.insert(record):
    bulk.finish()



Elastic search features to remember

GET /_search
{
  "from": 10,   "size": 10,
  "track_total_hits": true,
  "query": {     "match_all": {}   },
  "sort": [      {"fieldname":{"order":"asc"}}  ]
}

# creating the frozen work item list copy
POST /my_list_pfk/_close
DELETE /my_list_frozen
POST /my_list_pfk/_clone/my_list_frozen
POST /my_list_frozen/_close
POST /my_list_pfk/_open
