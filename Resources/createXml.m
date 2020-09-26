close all
clear

speakerAzEl = [
0	90
0	0
0	-90
0	45
0	-45
18.4	17.5
18.4	-17.5
45	0
45	64.8
45	35.3
45	-35.3
45	-64.8
71.6	17.5
71.6	-17.5
90	0
90	45
90	-45
108.4	17.5
108.4	-17.5
135	0
135	64.8
135	35.3
135	-35.3
135	-64.8
161.6	17.5
161.6	-17.5
180	0
180	45
180	-45
-161.6	17.5
-161.6	-17.5
-135	0
-135	64.8
-135	35.3
-135	-35.3
-135	-64.8
-108.4	17.5
-108.4	-17.5
-90	0
-90	45
-90	-45
-71.6	17.5
-71.6	-17.5
-45	64.8
-45	35.3
-45	-35.3
-45	-64.8
-45	0
-18.4	17.5
-18.4	-17.5
];

headers = {'ID','speakerAz','speakerEl','angDev','speakerDist','distDev'};
width = [50, 80, 80, 70, 80, 70];

%% SAVE CONFIG FILE
fileID = fopen('speaker_angles.xml', 'w');
fprintf(fileID,'<TABLE_DATA>\n');
fprintf(fileID,'    <HEADERS>\n');
for i = 1:length(headers)
    fprintf(fileID,'        <COLUMN columnId="%.0f" name="%s" width="%.0f"/>\n', i, string(headers(i)), width(i));
end
fprintf(fileID,'    </HEADERS>\n');
fprintf(fileID,'    <DATA>\n');
for i = 1:length(speakerAzEl)
    params = sprintf('%s="%.0f"', string(headers(1)), i);
    params = [params ' ' sprintf('%s="%.2f"', string(headers(2)), speakerAzEl(i,1))];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(3)), speakerAzEl(i,2))];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(4)), 5)];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(5)), 1.2)];
    params = [params ' ' sprintf('%s="%.2f"', string(headers(6)), 0.1)];
    fprintf(fileID,['        <ITEM ' params '/>\n']);
end
fprintf(fileID,'    </DATA>\n');
fprintf(fileID,'</TABLE_DATA>\n');
fclose(fileID);