function C = file2cell(aFileName)
% Reads text file (named aFileName) into a cell array

fid = fopen(aFileName, 'r');

k = 1;

while feof(fid) == 0
    tline = fgetl(fid);

    C{k} = str2num(tline);
    k = k + 1;
end

