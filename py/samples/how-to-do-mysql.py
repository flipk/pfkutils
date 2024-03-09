
def lookup_features(db,_id):
    query = "SELECT shitload,of,fields " + \
            " FROM some_table WHERE s.id = %u" % (_id)

    result = db.execute_query(query)

    if len(result)==1 and len(result[0])==24:
        return {
            'field 1'       :       result[0][ 0],
            'field 2'       :       result[0][ 1],
            'field 3'       : 0. if result[0][ 6]==None else float(result[0][ 6])
        }

    return None

import numpy as np

def offset_by_hz(iq, hz=7500.):
    t = np.arange(0, iq.shape[0], dtype=np.float64)
    return iq * np.exp(-1j * 2 * np.pi * t * hz / 30720000.0)

