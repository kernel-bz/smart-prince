/**
 *	file name:  ml/include/classifier_thread.h
 *	editted by: Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   classifier Thread Module
 */

#ifndef __CLASSIFIER_H
#define __CLASSIFIER_H

void* classifier_thread_local(void* arg);
void* classifier_thread_server(void* arg);
void* classifier_thread_train(void* arg);

void classifier_thread_run(char *datfile , char *cfgfile, char *wtsfile);
void classifier_train_run(char *datfile, char *cfgfile, char *wtsfile);
void classifier_validate_run(char *datfile, char *cfgfile, char *wtsfile);

#endif
