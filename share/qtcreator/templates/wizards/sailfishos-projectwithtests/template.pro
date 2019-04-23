TEMPLATE = subdirs
SUBDIRS = app tests

CONFIG += ordered

OTHER_FILES += $$files(rpm/*)
