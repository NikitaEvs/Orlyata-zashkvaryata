
# coding: utf-8

# In[33]:


import pandas as pd
import numpy as np
import glob, os


# In[48]:


try:
    q = pd.read_csv('learn/out.csv')
    w = pd.read_csv('learn/out1.csv')
    e = pd.read_csv('learn/out2.csv')
    r = pd.read_csv('learn/out3.csv')
    t = pd.read_csv('learn/out4.csv')
    y = pd.read_csv('learn/out5.csv')
    u = pd.read_csv('learn/out6.csv')
    i = pd.read_csv('learn/out7.csv')
    o = pd.read_csv('learn/out8.csv')
    p = pd.read_csv('learn/out9.csv')
    a = pd.read_csv('learn/out10.csv')
    s = pd.read_csv('learn/out11.csv')
    train_df = pd.concat([q,w,e,r,t,y,u,i,o,p,a,s])
except FileNotFoundError:
    print("kek")


# In[6]:


x = train_df.drop('Ans', axis = 1)
y = train_df.Ans


# In[8]:


from catboost import CatBoostClassifier, Pool, cv
from sklearn.metrics import accuracy_score
import catboost
model = CatBoostClassifier(
    custom_loss=['Accuracy'],
    logging_level='Silent',
    loss_function='MultiClass'
)


# In[17]:


from sklearn.model_selection import train_test_split

x_train, x_validation, y_train, y_validation = train_test_split(x, y, train_size=0.75, random_state=27)


# In[18]:


train_pool = Pool(x_train, y_train)
validate_pool = Pool(x_validation, y_validation)


# In[19]:


model.fit(
    x_train, y_train,
    eval_set=(x_validation, y_validation),
);


# In[20]:


from sklearn.metrics import accuracy_score

print('Train Accuracy:', accuracy_score(y_train, model.predict(x_train)))
print('Validation Accuracy:', accuracy_score(y_validation, model.predict(x_validation)))


# In[42]:


while 1:
    for file in glob.glob("data/*.csv"):
        x_test = pd.read_csv(file)
        Ans = pd.DataFrame()
        Ans['I'] = X_test['12']
        Ans['Ans'] = model.predict(X_test)
        Ans.to_csv(file.name, index=False)

