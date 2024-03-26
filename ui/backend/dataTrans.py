from datetime import datetime

def translate(date_string):
    try:
        datetime_obj = datetime.strptime(date_string, "%Y-%m-%d %H:%M:%S.%f")
    except ValueError:
        try:
            datetime_obj = datetime.strptime(date_string, "%Y-%m-%d %H:%M:%S")
        except:
            return ""

    timestamp = datetime_obj.timestamp()

    return str(timestamp)

if __name__ == "__main__":
    date_string = "2018-03-15 20:59:33.621793"
    print(translate(date_string))
    date_string = "2018-03-15 20:59:33"
    print(translate(date_string))