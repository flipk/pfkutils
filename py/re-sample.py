
import re

        # noinspection RegExpSimplifiable
        m = re.search('^([A-Za-z][0-9P]+)(-([0-9]+).*|.*-([0-9]+).*)$', mtg)
        if not m:
            notparsed.peg()
        else:
            wg = m.group(1)
            g3 = m.group(3)
            g4 = m.group(4)
            if g4:
                num = int(g4)
            else:
                num = int(g3)
            output.write("%s: %s %d\n" % (mtg, wg, num))
