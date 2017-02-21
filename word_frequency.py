import sys
import datetime

import matplotlib.pyplot as plt
import matplotlib.dates as mdates

params = [1, 0, 636928, 2567793, 909030, 4151203, 1128352, 3152829,
          2068697, 2587417, 2052571, 3764501, 1619340, 2920635, 868570, 3079234]
sketches = []

def string_hash(raw_string):
    hash = 5381;
    for char in raw_string:
        hash = ((hash << 5) + hash) + ord(char); # hash * 33 + c
        hash = hash & 0xffffffffffffffff # Trick to simulate unsigned long bit shifts
    return hash

def linear_hash(raw_int, a, b):
    m = 4194303
    return ((((raw_int * a) & 0xffffffff) + b) & 0xffffffff) % m

def look_up_min(raw_int):
    min = int(sketches[0][linear_hash(raw_int, params[0], params[1])])
    for i in range(0, 8): # TODO: CHANGE THIS
        hash = linear_hash(raw_int, params[i * 2], params[(i * 2) + 1])
        estimate = sketches[i][hash]
        if (estimate < min):
            min = estimate
    return min

def create_graph(dates, values, begin_date, end_date):
    # Plot the data

    years = mdates.YearLocator()   # every year
    months = mdates.MonthLocator()  # every month
    yearsFmt = mdates.DateFormatter('%Y')

    fig, ax = plt.subplots()

    ax.plot(dates, values)

    # format the ticks
    ax.xaxis.set_major_locator(years)
    ax.xaxis.set_major_formatter(yearsFmt)
    ax.xaxis.set_minor_locator(months)

    datemin = datetime.date(begin_date.year, begin_date.month, 1)
    datemax = datetime.date(end_date.year, end_date.month, 1)
    ax.set_xlim(datemin, datemax)

    ax.format_xdata = mdates.DateFormatter('%Y-%m-%d')
    ax.grid(True)

    # rotates and right aligns the x labels, and moves the bottom of the
    # axes up to make room for them
    fig.autofmt_xdate()

    plt.show()


###############################
# Prints and plots usage data #
###############################

if len(sys.argv) < 3:
    print("Usage: word_frequency <word> <CMS_file> <begin_date: yyyy:mm> <end_date: yyyy:mm>")
    print("Default begin date is 2002:01, default end date is now")

now = datetime.datetime.now()
begin_year = 2002
begin_month = 1
end_year = now.year
end_month = now.month

word = sys.argv[1]
cms_file = sys.argv[2]

if len(sys.argv) > 3:
    begin_year = int(sys.argv[3].split(":")[0])
    begin_month = int(sys.argv[3].split(":")[1])
if len(sys.argv) > 4:
    end_year = int(sys.argv[4].split(":")[0])
    end_month = int(sys.argv[4].split(":")[1])

for i in range(0, 8):
    f = open(cms_file + str(i))
    sketch = f.read().split(" ")
    sketches.append(sketch)

total_sum = 0
all_dates = []
all_values = []
hash = string_hash(sys.argv[1])

for year in range(begin_year, end_year + 1):
    annual_sum = 0
    annual_data = {}
    first_month = 1
    last_month = 12
    if (year == begin_year):
        first_month = begin_month
    if (year == end_year):
        last_month = end_month
    for month in range(first_month, last_month + 1):
        estimate = look_up_min((hash + (year * 12) + month) & 0xffffffff)
        if (estimate > 0):
            # print(str(year) + ":" + str(month) + " " + str(estimate))
            annual_data[month] = estimate
            annual_sum += estimate
            all_dates.append(datetime.datetime(year, month, 1))
            all_values.append(estimate)
    total_sum += annual_sum
    print(str(year) + ": total: " + str(annual_sum) + " breakdown: " + str(annual_data))
print("Total: " + str(total_sum))

create_graph(all_dates, all_values, datetime.datetime(begin_year, begin_month, 1),
             datetime.datetime(end_year, end_month, 1))