
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <stack>
#include <cstdlib>
#include <algorithm>
#include <sys/time.h>

#include "cog.hpp"
#include "iterator.hpp"
#include "cog_tester.hpp"
#include "rewrite.hpp"
#include "policy.hpp"

using namespace std;

Buffer build_buffer(int len, int max)
{
  int i;
  Buffer buff(new vector<Record>(len));
  for(i = 0; i < len; i++){
    (*buff)[i].key = rand() % max;
    (*buff)[i].value = (Value)0xDEADBEEF;
  }
  return buff;
}

Buffer load_buffer(istream &input)
{
  vector<Record> temp;
  Record r;
  r.value = (Value)0xDEADBEEF;
  while(!input.eof()){
    input >> r.key;
    temp.push_back(r);
  }
  return Buffer(new vector<Record>(temp));
}

CogHandle array_for_buffer(Buffer buff)
{
  return MakeHandle(new ArrayCog(buff, buff->begin(), buff->end()));
}


CogHandle build_random_array(int len, int max)
{
  return array_for_buffer(build_buffer(len, max));
}

CogHandle build_random_sorted_array(int len, int max)
{
  Buffer buff = build_buffer(len, max);
  sort(buff->begin(), buff->end(), CompareRecord());
  return MakeHandle(new SortedArrayCog(buff, buff->begin(), buff->end()));
}

void cog_test(istream &input)
{
  shared_ptr<string> curr;
  stack<CogHandle> stack;
  string line;
  RewritePolicy policy(new RewritePolicyBase()); // dumb empty policy
  
  while(getline(input, line)){
    istringstream toks(line);
    string op;
    
    toks >> op;
    
    ///////////////// COG LOADERS /////////////////
    if(string("array") == op){
      string fill;
      toks >> fill;
      
      if(string("random") == fill) {
        int len, max;
        toks >> len >> max;
        stack.push(build_random_array(len,max));        
      } else if(string("explicit") == fill) {
        stack.push(array_for_buffer(load_buffer(toks)));
      } else {
        cerr << "Invalid ArrayCog Fill Mode: " << fill << endl;
        exit(-1);
      }
      
    } else if(string("concat") == op) {
      CogHandle a = stack.top(); stack.pop();
      CogHandle b = stack.top(); stack.pop();
      
      stack.push(MakeHandle(new ConcatCog(a, b)));
    } else if(string("btree") == op) {
      Key sep;
      toks >> sep;
      
      CogHandle b = stack.top(); stack.pop();
      CogHandle a = stack.top(); stack.pop();

      stack.push(MakeHandle(new BTreeCog(a, sep, b)));
    
    ///////////////// SIMPLE QUERIES /////////////////
    } else if(string("size") == op) {
      cout << "Size: " << stack.top()->size() << " records" << endl; 
    } else if(string("dump") == op) {
      cout << "gROOT" << endl;
      stack.top()->printDebug(1);
    } else if(string("scan") == op) {
      CogHandle root = stack.top();
      policy->beforeIterator(root);
      Iterator iter = root->iterator(policy);
      int row = 1;
      cout << "---------------" << endl;
      while(!iter->atEnd()){
        cout << row << " -> " << iter->key() << endl;
        iter->next();
        row++;
      }
      cout << "---------------" << endl;
    } else if(string("time_scan") == op) {
      CogHandle root = stack.top();
      timeval start, end;
      gettimeofday(&start, NULL);
      policy->beforeIterator(root);
      Iterator iter = root->iterator(policy);
      int row = 1;
      while(!iter->atEnd()){ iter->next(); row++; }
      gettimeofday(&end, NULL);
      float totalTime = 
        (end.tv_sec - start.tv_sec) * 1000000.0 +
        (end.tv_usec - start.tv_usec);
      cout << "---------------" << endl;
      cout << "Records Scanned: " << (row-1) << endl;
      cout << "Time Taken: " << totalTime << " us" << endl;
      if(row > 1){
        cout << "Time/Record: " << totalTime/(row-1) << " us" << endl;
      }
      cout << "---------------" << endl;
      
    ///////////////// REWRITE OPERATIONS /////////////////
    } else if(string("split_array") == op) {
      Key target;
      toks >> target;
      splitArray(target, stack.top());
    } else if(string("rec_split_array") == op) {
      Key target;
      toks >> target;
      recurToTargetTopDown(makeSplitArray(target), target, stack.top());
    } else if(string("sort_array") == op) {
      sortArray(stack.top());
    } else if(string("rec_sort_array") == op) {
      recurTopDown(sortArray, stack.top());
    } else if(string("pushdown_array") == op) {
      pushdownArray(stack.top());
    } else if(string("rec_pushdown_array") == op) {
      recurTopDown(pushdownArray, stack.top());
    } else if(string("tgt_pushdown_array") == op) {
      Key target;
      toks >> target;
      recurToTarget(pushdownArray, target, stack.top());

    ///////////////// POLICY OPERATIONS /////////////////
    } else if(string("policy") == op){
      string policyName;
      toks >> policyName;
      if(string("naive") == policyName){
        policy = RewritePolicy(new RewritePolicyBase());
      } else if(string("cracker") == policyName){
        int minSize;
        toks >> minSize;
        policy = RewritePolicy(new CrackerPolicy(minSize));
      }
      cout << "Now using policy '" << policyName << "' -> '"  
           << policy->name() << "'" << endl;
    
    ///////////////// OOOPS /////////////////
    } else {
      cerr << "Invalid Test Operation: " << op << endl;
      exit(-1);
    }
  }
}