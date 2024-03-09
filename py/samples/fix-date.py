    def _fix_date(self, date: str) -> Union[str, None]:
        """
        parse a date in busted format and convert two
        digit year to four digit year following a >70 heuristic.
        :param date: the date in MDB format MM/DD/YY HH:MM:SS
        :return: the date in MM/DD/YYYY HH:MM:SS format
        """
        # match a string like "08/25/06 09:23:31"
        m = re.search('^(\\d+)/(\\d+)/(\\d+) (\\d+):(\\d+):(\\d+)$',
                      date)
        if not m:
            return None

        # because the regex matched \d, we know it's safe
        # to do int()
        month = int(m.group(1))
        day = int(m.group(2))
        year = int(m.group(3))
        hour = int(m.group(4))
        minute = int(m.group(5))
        second = int(m.group(6))

        # MEAT: fix year with a heuristic.
        if year < 1900:
            if year > 70:
                year += 1900
            else:
                year += 2000

        if year < self._minimum_year:
            self._skips.peg()
            return None

        # now re-encode into a string.
        return "%02u/%02u/%04u %02u:%02u:%02u" % (month, day, year, hour, minute, second)
