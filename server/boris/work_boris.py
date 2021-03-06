import pandas as pd
import numpy as np
import glob, os
import catboost
import sys
from catboost import CatBoostClassifier, Pool, cv

count = len(sys.argv) # numb for console arguments
name = '' # name for working model
if count == 2 :
	name = str(sys.argv[1])
else :
	print('Invalid model name')
	sys.exit(0)

model = CatBoostClassifier()
model.load_model(name)

root = os.path.dirname(os.path.abspath(__file__))

while 1:
        for file in os.listdir(root+"/../data"):
                if file.endswith(".csv"):
                        x_test = pd.read_csv(os.path.join(root+"/../data", file))
                        Ans = pd.DataFrame()
                        Ans['I'] = x_test['11']
                        Ans['Ans'] = model.predict(x_test)
                        Ans.to_csv(root+"/../ans/ans"+file, index=False)
                        os.remove(root+"/../data/"+file)



