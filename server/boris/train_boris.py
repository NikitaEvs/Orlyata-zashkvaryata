import sys, glob, os
import pandas as pd
import numpy as np

datasetPath = '' # string for name of using dataset
numberOfFile = 0 # numb for count of using file in dataset
count = len(sys.argv) # numb for console arguments
if count == 3 :
	datasetPath = str(sys.argv[1])
	numberOfFile = int(sys.argv[2])
else: 
	print('Please, write correct console argument with dataset name and number of csv file')
	sys.exit(0)
train_df = pd.read_csv('learn/' + datasetPath + '/out.csv') 
try:
	for iterator in range(1, numberOfFile) :
		print('learn/' + datasetPath + '/out' + str(iterator) + '.csv')
		temp_df = pd.read_csv('learn/' + datasetPath + '/out' + str(iterator) + '.csv')
		train_df = pd.concat([train_df, temp_df])
except FileNotFoundError:
	print('Check your dataset')

x = train_df.drop('Ans', axis = 1)
y = train_df.Ans

from catboost import CatBoostClassifier, Pool, cv
from sklearn.metrics import accuracy_score
import catboost
model = CatBoostClassifier(
    custom_loss=['Accuracy'],
    logging_level='Silent',
    loss_function='MultiClass'
)

from sklearn.model_selection import train_test_split
x_train, x_validation, y_train, y_validation = train_test_split(x, y, train_size=0.75, random_state=27)
train_pool = Pool(x_train, y_train)
validate_pool = Pool(x_validation, y_validation)

model.fit(
    x_train, y_train,
    eval_set=(x_validation, y_validation),
);

from sklearn.metrics import accuracy_score
print('Train Accuracy:', accuracy_score(y_train, model.predict(x_train)))
print('Validation Accuracy:', accuracy_score(y_validation, model.predict(x_validation)))
print('Save current model? Y/n')
ans = input()
if ans == 'n' : 
	print('Model will not save')
	sys.exit(0)
else :
	print('Write name of model')
	name = input()
	#directory = 'model/' + name + '/'
	#if not os.path.exists(directory):
		#os.makedirs(directory)
	model.save_model(name, format='cbm', export_parameters=None)

