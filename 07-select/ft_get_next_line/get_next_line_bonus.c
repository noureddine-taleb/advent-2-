/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_bonus.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntaleb <ntaleb@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/14 14:29:49 by ntaleb            #+#    #+#             */
/*   Updated: 2022/10/29 10:42:26 by ntaleb           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <limits.h>
#include "get_next_line_bonus.h"

size_t	line_size(char *buffer)
{
	char	*new_line;

	new_line = ft_strchr(buffer, '\n');
	if (new_line)
		return (new_line - buffer + 1);
	return (ft_strlen(buffer));
}

char	*get_line(char **buffer)
{
	size_t	to_copy;
	size_t	buffer_len;
	char	*line;
	size_t	i;

	if (!*buffer)
		return (NULL);
	buffer_len = ft_strlen(*buffer);
	to_copy = line_size(*buffer);
	line = malloc(to_copy + 1);
	i = 0;
	while (i < to_copy)
	{
		line[i] = (*buffer)[i];
		i++;
	}
	line[i] = 0;
	ft_memmove(*buffer, *buffer + to_copy, buffer_len - to_copy + 1);
	if (to_copy == buffer_len)
	{
		free(*buffer);
		*buffer = NULL;
	}
	return (line);
}

char	*get_next_line(int fd)
{
	char			*buffer;
	static char		*lines[OPEN_MAX];
	char			*tmp;
	int				ret;

	buffer = malloc(BUFFER_SIZE + 1);
	if (fd < 0 || !buffer)
		return (free(buffer), NULL);
	ret = read(fd, buffer, BUFFER_SIZE);
	while (ret > 0)
	{
		buffer[ret] = 0;
		if (lines[fd])
		{
			tmp = ft_strjoin(lines[fd], buffer);
			free(lines[fd]);
			lines[fd] = tmp;
		}
		else
			lines[fd] = ft_strdup(buffer);
		if (!lines[fd] || ft_strchr(lines[fd], '\n'))
			break ;
		ret = read(fd, buffer, BUFFER_SIZE);
	}
	return (free(buffer), get_line(&lines[fd]));
}
