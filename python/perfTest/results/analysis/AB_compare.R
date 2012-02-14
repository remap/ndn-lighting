# attempt at comparing two datasets for RTT network performance
# some taken from https://blogs.oracle.com/Eoin/entry/first_past_the_line_analyzing

baseFile <- read.table("results.asymm.forR.txt", header = FALSE,  col.names= c("basedata"))
testFile <- read.table("results.symm.forR.txt", header = FALSE,  col.names= c("testdata"))
attach(baseFile)
attach(testFile)
summary(basedata)
summary(testdata)
library(psych)
describe(basedata)
describe(testdata)
boxplot(basedata,testdata, main="Box plot of Asymm (L) and Symm (R) RTT")
#plot(basedata[0:41],testdata[0:41], main="Asymmetric vs Symmetric RTT")
plot.new()
par(mfcol=c(1,2) )  # to plot side by
barplot(basedata, xlab="Iteration", ylab="Result",ylim=c(0,datamax*1.1))
barplot(testdata, xlab="Iteration", ylab="Result",ylim=c(0,datamax*1.1))
dev.off(2)
plot.new()
stripchart(basedata, method="stack", group.names="asymm, symm", col="red",xlab="time (s)",main = "RTT Data Object distribution - Asymm vs Symmetric (blue)",pch=21)
stripchart(testdata, method="stack", add=TRUE, at=0.8, col="blue", offset=1,pch=21)
dev.off(2)

# test outliers:
library(outliers)
grubbs.test(basedata)
# if 1, no outliers

# test normality:
shapiro.test(basedata)
shapiro.test(testdata)
# if < .5, cannot use T test, instead use wilcoxon

# test to make sure we have enough data
sd<- max(sd(basedata),sd(testdata))
d<-mean(basedata)*0.01
power.t.test(sd=sd,sig.level=0.05,power=0.8,delta=d,type="two.sample")
# first result is how many samples for a one percent change
wilcox.test(testdata,basedata, conf.int=TRUE)
# is pval < .05 ? 
wilcox.test(testdata, basedata, conf.int=TRUE, conf.level=0.9)
