#!/usr/bin/env python

import graphviz
import random
import sys
import pandas as pd

from sklearn.svm import SVC, LinearSVC
from sklearn.feature_selection import SelectFromModel
from sklearn.gaussian_process import GaussianProcessClassifier
from sklearn.model_selection import LeaveOneOut
from sklearn.naive_bayes import GaussianNB
from sklearn.pipeline import Pipeline

def input_vars(df):
    return [cn for cn in df.columns if not cn.startswith('out')]

def input_data(df):
    return df.loc[:, input_vars(df)]

def output_var(df, var):
    return df.loc[:, var]

def model():
    return Pipeline([
        ('classification', GaussianNB())
    ])

def main(argv):
    path = argv[0]
    var = argv[1]

    data = pd.read_csv(path)
    xs, ys = input_data(data), output_var(data, var)

    loo = LeaveOneOut()
    correct = 0
    for train_idx, test_idx in loo.split(data):
        train_xs = xs.loc[train_idx]
        train_ys = ys.loc[train_idx]
        test_xs = xs.loc[test_idx]
        test_ys = ys.loc[test_idx]
        mod = model().fit(train_xs, train_ys)
        if all(mod.predict(test_xs) == test_ys):
            correct += 1
    print("{}: {:.2f}%".format(var, 100.0 * correct / len(data)))

if __name__ == "__main__":
    main(sys.argv[1:])
