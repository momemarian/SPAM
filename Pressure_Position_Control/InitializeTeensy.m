function [  ] = InitializeTeensy(tcpipConnection)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here


fwrite(tcpipConnection,'G');
tmpScan = fscanf (tcpipConnection,'%s');
tmpScan = fscanf (tcpipConnection,'%s');
end

